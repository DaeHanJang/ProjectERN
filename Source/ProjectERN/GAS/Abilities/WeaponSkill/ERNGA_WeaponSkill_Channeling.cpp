// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Abilities/WeaponSkill/ERNGA_WeaponSkill_Channeling.h"

#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Combat/Weapons/ERNMeleeWeapon.h"
#include "Components/BoxComponent.h"
#include "DrawDebugHelpers.h"
#include "Character/Player/ERNSkillNiagaraComponent.h"
#include "Character/Player/ProjectERNCharacter.h"
#include "Combat/ERNSkillDamageLibrary.h"
#include "Components/AudioComponent.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/Character.h"
#include "Inventory/Components/ERNEquipmentComponent.h"
#include "Kismet/GameplayStatics.h"

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
	
	bEndChannelingRequested = false;
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
	// 사운드 적용
	StartChannelingSound(MeshComp);
	
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
	// 사운드 정지
	StopChannelingSound();
	
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
	
	if (bEndChannelingRequested || IsAttackMontageInSection(ChannelingData.EndSectionName))
	{
		return;
	}

	bEndChannelingRequested = true;
	
	StopChanneling();

	if (!IsActive())
	{
		return;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC || !AttackMontage || ChannelingData.EndSectionName == NAME_None)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	const bool bHasAuthority = AvatarActor && AvatarActor->HasAuthority();

	// 클라이언트에 종료요청
	if (bHasAuthority)
	{
		if (AProjectERNCharacter* Character = Cast<AProjectERNCharacter>(AvatarActor))
		{
			Character->Client_RequestEndActiveChannelingWeaponSkill();
		}
	}
	
	if (!bHasAuthority)
	{
		if (AvatarActor)
		{
			if (USkeletalMeshComponent* Mesh = AvatarActor->FindComponentByClass<USkeletalMeshComponent>())
			{
				if (UAnimInstance* AnimInstance = Mesh->GetAnimInstance())
				{
					if (AnimInstance->Montage_IsPlaying(AttackMontage))
					{
						AnimInstance->Montage_JumpToSection(ChannelingData.EndSectionName, AttackMontage);
					}
				}
			}
		}

		return;
	}

	// 서버는 GAS CurrentMontage 경로로 처리
	if (ASC->GetCurrentMontage() != AttackMontage)
	{
		// 여기로 들어오면 1번 방식(GAS 몽타주 복제)이 성립하지 않는 상태
		ASC->CurrentMontageStop(0.15f);
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	if (ChannelingData.LoopSectionName != NAME_None)
	{
		ASC->CurrentMontageSetNextSectionName(ChannelingData.LoopSectionName, ChannelingData.EndSectionName);
	}

	ASC->CurrentMontageJumpToSection(ChannelingData.EndSectionName);
}

void UERNGA_WeaponSkill_Channeling::EndAbility(const FGameplayAbilitySpecHandle Handle,
                                               const FGameplayAbilityActorInfo* ActorInfo,
                                               const FGameplayAbilityActivationInfo ActivationInfo,
                                               bool bReplicateEndAbility, 
                                               bool bWasCancelled)
{
	StopChanneling();
	StopChannelingSound();
	bEndChannelingRequested = false;
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
		AActor* HitActor = Result.GetActor();
		if (!HitActor || HitActor == OwnerActor || DamagedActors.Contains(HitActor))
		{
			continue;
		}

		// 스킬 공격 전달
		const EERNSkillHitResult HitResult = UERNSkillDamageLibrary::ApplySkillHit(
			HitActor,
			OwnerActor,
			InstigatorController,
			ChannelingData.DamageData,
			Origin);

		if (HitResult != EERNSkillHitResult::None)
		{
			DamagedActors.Add(HitActor);
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

bool UERNGA_WeaponSkill_Channeling::IsAttackMontageInSection(FName SectionName) const
{
	if (!AttackMontage || SectionName == NAME_None)
	{
		return false;
	}

	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!AvatarActor)
	{
		return false;
	}

	USkeletalMeshComponent* Mesh = AvatarActor->FindComponentByClass<USkeletalMeshComponent>();
	UAnimInstance* AnimInstance = Mesh ? Mesh->GetAnimInstance() : nullptr;

	return AnimInstance 
	&& AnimInstance->Montage_IsPlaying(AttackMontage) 
	&& AnimInstance->Montage_GetCurrentSection(AttackMontage) == SectionName;
}

bool UERNGA_WeaponSkill_Channeling::CanRequestEndChanneling() const
{
	return !bEndChannelingRequested && IsAttackMontageInSection(ChannelingData.LoopSectionName);
}

void UERNGA_WeaponSkill_Channeling::StartChannelingSound(USkeletalMeshComponent* MeshComp)
{
	if (ActiveChannelingAudioComponent || !ChannelingData.ChannelingLoopSound || !MeshComp)
	{
		return;
	}

	USceneComponent* AttachComponent = MeshComp;
	FTransform OriginTransform;

	if (USceneComponent* CalculatedAttach = nullptr;
		GetChannelOriginTransform(MeshComp, OriginTransform, CalculatedAttach) && CalculatedAttach)
	{
		AttachComponent = CalculatedAttach;
	}

	const float InitialVolume = ChannelingData.ChannelingSoundFadeInTime > 0.f
		? 0.f
		: ChannelingData.ChannelingSoundVolume;

	ActiveChannelingAudioComponent = UGameplayStatics::SpawnSoundAttached(
		ChannelingData.ChannelingLoopSound,
		AttachComponent,
		NAME_None,
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		EAttachLocation::SnapToTarget,
		false,
		InitialVolume,
		ChannelingData.ChannelingSoundPitch,
		0.f,
		ChannelingData.ChannelingSoundAttenuation);

	if (ActiveChannelingAudioComponent && ChannelingData.ChannelingSoundFadeInTime > 0.f)
	{
		ActiveChannelingAudioComponent->FadeIn(
			ChannelingData.ChannelingSoundFadeInTime,
			ChannelingData.ChannelingSoundVolume);
	}
}

void UERNGA_WeaponSkill_Channeling::StopChannelingSound()
{
	if (!ActiveChannelingAudioComponent)
	{
		return;
	}

	if (ChannelingData.ChannelingSoundFadeOutTime > 0.f)
	{
		ActiveChannelingAudioComponent->FadeOut(ChannelingData.ChannelingSoundFadeOutTime, 0.f);
	}
	else
	{
		ActiveChannelingAudioComponent->Stop();
	}

	ActiveChannelingAudioComponent = nullptr;
}
