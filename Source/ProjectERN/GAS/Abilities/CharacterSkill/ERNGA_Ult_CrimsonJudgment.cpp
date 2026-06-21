// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Abilities/CharacterSkill/ERNGA_Ult_CrimsonJudgment.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Combat/Weapons/ERNMeleeWeapon.h"
#include "Inventory/Components/ERNEquipmentComponent.h"

UERNGA_Ult_CrimsonJudgment::UERNGA_Ult_CrimsonJudgment()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UERNGA_Ult_CrimsonJudgment::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	
	ExecuteCastGameplayCue();

	UAbilityTask_PlayMontageAndWait* MontageTask = PlayConfiguredMontage(0);
	if (!MontageTask)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	MontageTask->OnCompleted.AddDynamic(this, &UERNGA_Ult_CrimsonJudgment::FinishSkill);
	// MontageTask->OnBlendOut.AddDynamic(this, &UERNGA_Ult_CrimsonJudgment::FinishSkill);
	MontageTask->OnInterrupted.AddDynamic(this, &UERNGA_Ult_CrimsonJudgment::FinishSkill);
	MontageTask->OnCancelled.AddDynamic(this, &UERNGA_Ult_CrimsonJudgment::FinishSkill);

	// 궁극기 동안 모든 클라에 큰 트레일로 교체 (서버 → 멀티캐스트, 토글 없이 한 번)
	ServerSetUltimateTrail(UltimateTrailEffect);
}

void UERNGA_Ult_CrimsonJudgment::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	// 궁극기 종료 시 기본 트레일로 복귀 (모든 클라)
	ServerSetUltimateTrail(nullptr);

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UERNGA_Ult_CrimsonJudgment::FinishSkill()
{
	if (IsActive())
	{
		K2_EndAbility();
	}
}

AERNMeleeWeapon* UERNGA_Ult_CrimsonJudgment::GetEquippedMeleeWeapon() const
{
	const AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!Avatar)
	{
		return nullptr;
	}

	UERNEquipmentComponent* Equipment = Avatar->FindComponentByClass<UERNEquipmentComponent>();
	if (!Equipment)
	{
		return nullptr;
	}

	return Cast<AERNMeleeWeapon>(Equipment->CurrentWeapon);
}

void UERNGA_Ult_CrimsonJudgment::ServerSetUltimateTrail(UNiagaraSystem* Trail)
{
	const AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!Avatar || !Avatar->HasAuthority())
	{
		return;	// 멀티캐스트는 서버에서만 발신
	}

	if (AERNMeleeWeapon* Weapon = GetEquippedMeleeWeapon())
	{
		Weapon->Multicast_SetBladeTrail(Trail);
	}
}
