// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Abilities/CharacterSkill/ERNGA_Ult_Sanctuary.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Actors/AoE/ERNAoE_Heal.h"
#include "Character/Player/ProjectERNCharacter.h"
#include "Kismet/GameplayStatics.h"

UERNGA_Ult_Sanctuary::UERNGA_Ult_Sanctuary()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UERNGA_Ult_Sanctuary::SpawnAoEFromNotify(USkeletalMeshComponent* MeshComp)
{
	AProjectERNCharacter* Caster = nullptr;
	if (!CanSpawnAoEFromNotify(MeshComp, Caster))
	{
		return;
	}

	UWorld* World = Caster->GetWorld();
	if (!World || !AoEActorClass)
	{
		return;
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

	// 원하는 변수를 적용하여 액터를 스폰하기 위해 SpawnActorDeferred를 사용
	AERNAoE_Heal* AoEActor = World->SpawnActorDeferred<AERNAoE_Heal>(
		AoEActorClass,
		SpawnTransform,
		Caster,
		Caster,
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

	if (!AoEActor)
	{
		return;
	}

	// Actor Spawn 전 Caster 세팅
	AoEActor->InitializeAoE(Caster);
	UGameplayStatics::FinishSpawningActor(AoEActor, SpawnTransform);

	// 장판 생성 VFX/SFX Cue
	ExecuteUltimateGameplayCue(
		AoECueData,
		Caster,
		Origin,
		Rotation);
	
	// 중복 방지
	if (bSpawnAoEOnlyOncePerActivation)
	{
		bAoESpawnedThisActivation = true;
	}
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
