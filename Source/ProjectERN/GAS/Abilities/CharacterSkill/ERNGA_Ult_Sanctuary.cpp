// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Abilities/CharacterSkill/ERNGA_Ult_Sanctuary.h"

#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Actors/AoE/ERNAoE_Heal.h"
#include "Character/Player/ProjectERNCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "GAS/ERNGameplayTags.h"
#include "Kismet/GameplayStatics.h"

UERNGA_Ult_Sanctuary::UERNGA_Ult_Sanctuary()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UERNGA_Ult_Sanctuary::SpawnAoEFromNotify(USkeletalMeshComponent* MeshComp)
{
	TrySpawnAoE(MeshComp);
}

void UERNGA_Ult_Sanctuary::OnSpawnAoEEventReceived(FGameplayEventData Payload)
{
	if (!IsSpawnAoEEventFromAvatar(Payload))
	{
		return;
	}

	TrySpawnAoE(GetAvatarMeshFromActorInfo());
}

bool UERNGA_Ult_Sanctuary::TrySpawnAoE(USkeletalMeshComponent* MeshComp)
{
	AProjectERNCharacter* Caster = nullptr;
	if (!CanSpawnAoEFromNotify(MeshComp, Caster))
	{
		return false;
	}

	UWorld* World = Caster->GetWorld();
	if (!World || !AoEActorClass)
	{
		return false;
	}

	const FVector Origin =
		Caster
			? (Caster->GetActorLocation()
				+ Caster->GetActorForwardVector() * AoEOriginOffset.X
				+ Caster->GetActorRightVector() * AoEOriginOffset.Y
				+ Caster->GetActorUpVector() * AoEOriginOffset.Z)
			: FVector::ZeroVector;
	const FRotator Rotation = Caster->GetActorRotation();
	const FTransform SpawnTransform(Rotation, Origin);

	AERNAoE_Heal* AoEActor = World->SpawnActorDeferred<AERNAoE_Heal>(
		AoEActorClass,
		SpawnTransform,
		Caster,
		Caster,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

	if (!AoEActor)
	{
		return false;
	}

	AoEActor->InitializeAoE(Caster);
	UGameplayStatics::FinishSpawningActor(AoEActor, SpawnTransform);

	if (bReviveDownedAlliesOnAoESpawn)
	{
		ReviveDownedAlliesInRadius(Origin, Caster);
	}

	ExecuteUltimateGameplayCue(
		AoECueData,
		Caster,
		Origin,
		Rotation);

	if (bSpawnAoEOnlyOncePerActivation)
	{
		bAoESpawnedThisActivation = true;
	}

	return true;
}

bool UERNGA_Ult_Sanctuary::IsSpawnAoEEventFromAvatar(const FGameplayEventData& Payload) const
{
	const AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!AvatarActor)
	{
		return false;
	}

	const AActor* EventInstigator = Payload.Instigator.Get();
	const AActor* EventTarget = Payload.Target.Get();

	return (!EventInstigator || EventInstigator == AvatarActor) &&
		(!EventTarget || EventTarget == AvatarActor);
}

USkeletalMeshComponent* UERNGA_Ult_Sanctuary::GetAvatarMeshFromActorInfo() const
{
	const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
	if (ActorInfo && ActorInfo->SkeletalMeshComponent.IsValid())
	{
		return ActorInfo->SkeletalMeshComponent.Get();
	}

	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	return AvatarActor ? AvatarActor->FindComponentByClass<USkeletalMeshComponent>() : nullptr;
}

void UERNGA_Ult_Sanctuary::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                           const FGameplayAbilityActorInfo* ActorInfo,
                                           const FGameplayAbilityActivationInfo ActivationInfo,
                                           const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	bAoESpawnedThisActivation = false;

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UAbilityTask_WaitGameplayEvent* SpawnAoEEventTask =
		UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
			this,
			TAG_Event_Skill_Ultimate_Sanctuary_SpawnAoE);

	SpawnAoEEventTask->EventReceived.AddDynamic(
		this,
		&UERNGA_Ult_Sanctuary::OnSpawnAoEEventReceived);
	SpawnAoEEventTask->ReadyForActivation();

	// 시전 시작 VFX/SFX Cue
	ExecuteCastGameplayCue();

	UAbilityTask_PlayMontageAndWait* MontageTask = PlayConfiguredMontage(0);
	if (!MontageTask)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	MontageTask->OnCompleted.AddDynamic(this, &UERNGA_Ult_Sanctuary::FinishSkill);
	MontageTask->OnBlendOut.AddDynamic(this, &UERNGA_Ult_Sanctuary::FinishSkill);
	MontageTask->OnInterrupted.AddDynamic(this, &UERNGA_Ult_Sanctuary::FinishSkill);
	MontageTask->OnCancelled.AddDynamic(this, &UERNGA_Ult_Sanctuary::FinishSkill);
}

bool UERNGA_Ult_Sanctuary::CanSpawnAoEFromNotify(USkeletalMeshComponent* MeshComp,
                                                 AProjectERNCharacter*& OutCaster) const
{
	OutCaster = nullptr;

	// 실행 중인 GA인지 확인
	if (!IsActive())
	{
		return false;
	}

	// 중복 소환 여부 확인
	if (bSpawnAoEOnlyOncePerActivation && bAoESpawnedThisActivation)
	{
		return false;
	}

	// 캐릭터 확인
	AProjectERNCharacter* Caster = Cast<AProjectERNCharacter>(GetAvatarActorFromActorInfo());
	if (!Caster)
	{
		return false;
	}

	// 실제 장판 생성과 힐 판정은 서버에서만 처리
	if (!Caster->HasAuthority())
	{
		return false;
	}

	// 다른 캐릭터의 몽타주 Notify가 잘못 들어오는 경우 방지
	if (MeshComp && MeshComp->GetOwner() != Caster)
	{
		return false;
	}

	OutCaster = Caster;
	return true;
}

void UERNGA_Ult_Sanctuary::FinishSkill()
{
	if (IsActive())
	{
		K2_EndAbility();
	}
}

int32 UERNGA_Ult_Sanctuary::ReviveDownedAlliesInRadius(const FVector& Origin, AProjectERNCharacter* Caster) const
{
	// 서버에서 실행, 적용 범위 실수로 존재
	if (!Caster || !Caster->HasAuthority() || ReviveRadius <= 0.f)
	{
		return 0;
	}

	UWorld* World = Caster->GetWorld();
	if (!World)
	{
		return 0;
	}

	AController* Reviver = Caster->GetController();
	const float ReviveRadiusSq = FMath::Square(ReviveRadius);

	int32 RevivedCount = 0;

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		// PC가 소유한 Pawn이 AProjectERNCharacter일 때만 적용
		APlayerController* PlayerController = It->Get();
		AProjectERNCharacter* TargetCharacter = PlayerController ? Cast<AProjectERNCharacter>(PlayerController->GetPawn()) : nullptr;

		// 시전자는 적용하지 않음
		if (!TargetCharacter || TargetCharacter == Caster)
		{
			continue;
		}

		// 기절 상태에만 적용
		if (!TargetCharacter->IsDowned())
		{
			continue;
		}

		if (FVector::DistSquared2D(TargetCharacter->GetActorLocation(), Origin) > ReviveRadiusSq)
		{
			continue;
		}

		// 대상에게 부활 적용
		if (TargetCharacter->TryReviveFromSkill(Reviver))
		{
			++RevivedCount;
		}
	}

	return RevivedCount;
}
