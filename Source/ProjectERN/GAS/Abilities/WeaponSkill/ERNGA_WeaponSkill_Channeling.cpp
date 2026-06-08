// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Abilities/WeaponSkill/ERNGA_WeaponSkill_Channeling.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Character/Enemy/ERNEnemyCharacter.h"
#include "Combat/Weapons/ERNWeaponBase.h"
#include "Combat/Weapons/ERNMeleeWeapon.h"
#include "Components/BoxComponent.h"
#include "DrawDebugHelpers.h"
#include "Character/Player/ERNSkillNiagaraComponent.h"
#include "Engine/DamageEvents.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/Character.h"
#include "GAS/ERNAttributeSet.h"
#include "Inventory/Components/ERNEquipmentComponent.h"

UERNGA_WeaponSkill_Channeling::UERNGA_WeaponSkill_Channeling()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UERNGA_WeaponSkill_Channeling::StartChannelingFromNotify(USkeletalMeshComponent* MeshComp)
{
	// 채널링 중이라면 return (중복 방지)
	if (bIsChanneling)
	{
		return;
	}
	
	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	if (!ActorInfo || !ActorInfo->AbilitySystemComponent.IsValid() || !MeshComp)
	{
		return;
	}

	AActor* AvatarActor = ActorInfo->AvatarActor.Get();
	// 나이아가라 효과는 서버/클라 모두 적용
	if (!AvatarActor || MeshComp->GetOwner() != AvatarActor)
	{
		return;
	}
	
	// 타이머 쓰기 위함
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// 채널링 시작
	bIsChanneling = true;
	// 스킬 사용위치
	CachedMeshComp = MeshComp;
	// 누적 시간 초기화
	ChannelElapsedTime = 0.f;
	// 최소 0.05초 적용
	ActiveTickInterval = FMath::Max(0.05f, ChannelingData.TickInterval);

	// 채널링 효과 소환
	StartChannelingNiagaraEffects();

	// 대미지/코스트 tick은 서버에서만
	if (!AvatarActor->HasAuthority())
	{
		return;
	}
	
	// 채널링 타이머 호출
	World->GetTimerManager().SetTimer(
		ChannelTickTimerHandle,
		this,
		&UERNGA_WeaponSkill_Channeling::TickChanneling,
		ActiveTickInterval,
		true,
		0.f);
}

void UERNGA_WeaponSkill_Channeling::StopChanneling()
{
	// 타이머 초기화
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ChannelTickTimerHandle);
	}

	// 채널링 이펙트 제거
	StopChannelingNiagaraEffects();

	// 스킬 시작 위치 초기화
	CachedMeshComp.Reset();
	// 채널링 누적시간 초기화
	ChannelElapsedTime = 0.f;
	// 타이머 틱 간격 초기화
	ActiveTickInterval = 0.f;
	// 채널링 끝
	bIsChanneling = false;
}

void UERNGA_WeaponSkill_Channeling::RequestEndChanneling()
{
	StopChanneling();

	if (!IsActive())
	{
		return;
	}
	
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	// 섹션 이름이 적용되어 있지 않다면 바로 EndAbility
	if (!ASC || ChannelingData.LoopSectionName == NAME_None || ChannelingData.EndSectionName == NAME_None)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	// 현재 Loop를 기다리지 않고 즉시 End로 이어지게 한다.
	ASC->CurrentMontageJumpToSection(ChannelingData.EndSectionName);
	// 현재 Loop가 끝나면 End 섹션으로 이어지게 한다.
	// ASC->CurrentMontageSetNextSectionName(ChannelingData.LoopSectionName,ChannelingData.EndSectionName);
}

void UERNGA_WeaponSkill_Channeling::EndAbility(const FGameplayAbilitySpecHandle Handle,
                                               const FGameplayAbilityActorInfo* ActorInfo,
                                               const FGameplayAbilityActivationInfo ActivationInfo,
                                               bool bReplicateEndAbility, 
                                               bool bWasCancelled)
{
	StopChanneling();
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UERNGA_WeaponSkill_Channeling::TickChanneling()
{
	// 시작 위치가 없다면
	USkeletalMeshComponent* MeshComp = CachedMeshComp.Get();
	if (!MeshComp)
	{
		// 종료 요청
		RequestEndChanneling();
		return;
	}

	// 간격 조정
	const float TickInterval = ActiveTickInterval > 0.f
		? ActiveTickInterval
		: FMath::Max(0.05f, ChannelingData.TickInterval);

	// 채널링 지속 시간이 다 되면 종료
	if (ChannelingData.MaxChannelDuration > 0.f)
	{
		ChannelElapsedTime += TickInterval;
		if (ChannelElapsedTime >= ChannelingData.MaxChannelDuration)
		{
			// 종료 요청
			RequestEndChanneling();
			return;
		}
	}

	// 마나가 부족하면 중지
	if (ChannelingData.ManaCostPerTick > 0.f && !ApplyResourceCost(0.f, ChannelingData.ManaCostPerTick))
	{
		// 종료 요청
		RequestEndChanneling();
		return;
	}

	// 대미지 부여
	ApplyChannelDamage(MeshComp);
}

void UERNGA_WeaponSkill_Channeling::ApplyChannelDamage(USkeletalMeshComponent* MeshComp)
{
	if (ChannelingData.DamageLength <= 0.f || ChannelingData.DamageRadius <= 0.f)
	{
		return;
	}

	UWorld* World = GetWorld();
	AActor* OwnerActor = MeshComp ? MeshComp->GetOwner() : nullptr;
	if (!World || !OwnerActor || !OwnerActor->HasAuthority())
	{
		return;
	}

	FTransform OriginTransform;
	USceneComponent* AttachComponent = nullptr;
	if (!GetChannelOriginTransform(MeshComp, OriginTransform, AttachComponent))
	{
		return;
	}

	const FVector Origin = OriginTransform.GetLocation();
	const FVector Forward = OriginTransform.GetRotation().Rotator().Vector().GetSafeNormal();

	const float HalfLength = ChannelingData.DamageLength * 0.5f;
	const FVector Center = Origin + Forward * HalfLength;

	const FQuat CapsuleRotation = FRotationMatrix::MakeFromZ(Forward).ToQuat();

	TArray<FOverlapResult> Results;

	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(OwnerActor);

	const bool bHit = World->OverlapMultiByObjectType(
		Results,
		Center,
		CapsuleRotation,
		ObjectParams,
		FCollisionShape::MakeCapsule(ChannelingData.DamageRadius, HalfLength),
		QueryParams);

	const float DebugLifeTime = ActiveTickInterval > 0.f
		? ActiveTickInterval
		: FMath::Max(0.05f, ChannelingData.TickInterval);

	if (ChannelingData.bDrawDebug)
	{
		DrawDebugCapsule(
			World,
			Center,
			HalfLength,
			ChannelingData.DamageRadius,
			CapsuleRotation,
			FColor::Orange,
			false,
			DebugLifeTime,
			0,
			2.f);
	}

	if (!bHit)
	{
		return;
	}

	AController* InstigatorController = nullptr;
	if (const ACharacter* OwnerCharacter = Cast<ACharacter>(OwnerActor))
	{
		InstigatorController = OwnerCharacter->GetController();
	}

	TSet<AActor*> DamagedActors;

	for (const FOverlapResult& Result : Results)
	{
		AERNEnemyCharacter* Enemy = Cast<AERNEnemyCharacter>(Result.GetActor());
		if (!Enemy || DamagedActors.Contains(Enemy))
		{
			continue;
		}

		DamagedActors.Add(Enemy);

		// 적에게 계산된 대미지 전달
		Enemy->TakeDamage(
			CalculateDamage(OwnerActor),
			FDamageEvent(),
			InstigatorController,
			OwnerActor);

		if (ChannelingData.StaggerPower > 0.f)
		{
			// 채널 발원점(Origin)을 HitOrigin으로 전달 → 적이 4방향 경직
			Enemy->TryApplyStagger(ChannelingData.StaggerPower, Origin);
		}
	}
}

void UERNGA_WeaponSkill_Channeling::StartChannelingNiagaraEffects()
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!AvatarActor || ChannelingData.ChannelingNiagaraEffects.IsEmpty())
	{
		return;
	}

	UERNSkillNiagaraComponent* NiagaraComponent = AvatarActor->FindComponentByClass<UERNSkillNiagaraComponent>();

	if (!NiagaraComponent)
	{
		return;
	}

	NiagaraComponent->StartEffects(ChannelingData.ChannelingNiagaraEffects);
}

void UERNGA_WeaponSkill_Channeling::StopChannelingNiagaraEffects()
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!AvatarActor || ChannelingData.ChannelingNiagaraEffects.IsEmpty())
	{
		return;
	}

	UERNSkillNiagaraComponent* NiagaraComponent =
		AvatarActor->FindComponentByClass<UERNSkillNiagaraComponent>();

	if (!NiagaraComponent)
	{
		return;
	}

	NiagaraComponent->StopEffects(ChannelingData.ChannelingNiagaraEffects);
}

bool UERNGA_WeaponSkill_Channeling::GetChannelOriginTransform(USkeletalMeshComponent* MeshComp,
                                                              FTransform& OutTransform,
                                                              USceneComponent*& OutAttachComponent) const
{
	if (!MeshComp || !MeshComp->GetOwner())
	{
		return false;
	}

	AActor* OwnerActor = MeshComp->GetOwner();
	OutAttachComponent = MeshComp;

	if (ChannelingData.OriginData.OriginMode == EWeaponSkillAreaOriginMode::WeaponHitbox)
	{
		if (const UERNEquipmentComponent* Equipment = OwnerActor->FindComponentByClass<UERNEquipmentComponent>())
		{
			if (const AERNMeleeWeapon* Weapon = Cast<AERNMeleeWeapon>(Equipment->CurrentWeapon))
			{
				if (UBoxComponent* Hitbox = Weapon->GetHitboxComponent())
				{
					OutTransform = Hitbox->GetComponentTransform();
					OutAttachComponent = Hitbox;
				}
			}
		}
	}
	else if (ChannelingData.OriginData.OriginMode == EWeaponSkillAreaOriginMode::MeshSocket &&
		MeshComp->DoesSocketExist(ChannelingData.OriginData.MeshSocketName))
	{
		OutTransform = MeshComp->GetSocketTransform(ChannelingData.OriginData.MeshSocketName);
	}
	else
	{
		OutTransform = FTransform(OwnerActor->GetActorRotation(), OwnerActor->GetActorLocation());
		OutAttachComponent = OwnerActor->GetRootComponent();
	}

	OutTransform.SetLocation(OutTransform.TransformPosition(ChannelingData.OriginData.LocationOffset));
	OutTransform.ConcatenateRotation(ChannelingData.OriginData.RotationOffset.Quaternion());
	return true;
}

float UERNGA_WeaponSkill_Channeling::CalculateDamage(AActor* OwnerActor) const
{
	const float WeaponDamage = GetWeaponBaseDamage(OwnerActor);
	const float CharacterAttackPower = GetCharacterAttackPower(OwnerActor);
	
	return (WeaponDamage + CharacterAttackPower) * ChannelingData.DamageMultiplier;
}

float UERNGA_WeaponSkill_Channeling::GetWeaponBaseDamage(AActor* OwnerActor) const
{
	if (!OwnerActor)
	{
		return 0.f;
	}

	if (const UERNEquipmentComponent* Equipment =
		OwnerActor->FindComponentByClass<UERNEquipmentComponent>())
	{
		if (const AERNWeaponBase* Weapon = Equipment->CurrentWeapon)
		{
			return Weapon->LightAttackDamage;
		}
	}

	return 0.f;
}

float UERNGA_WeaponSkill_Channeling::GetCharacterAttackPower(AActor* OwnerActor) const
{
	if (!OwnerActor)
	{
		return 0.f;
	}

	const UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OwnerActor);

	if (!ASC)
	{
		return 0.f;
	}

	const float AttackPower = ASC->GetNumericAttribute(UERNAttributeSet::GetAttackPowerAttribute());

	const float AttackPowerBonus = ASC->GetNumericAttribute(UERNAttributeSet::GetAttackPowerBonusAttribute());

	return AttackPower + AttackPowerBonus;
}
