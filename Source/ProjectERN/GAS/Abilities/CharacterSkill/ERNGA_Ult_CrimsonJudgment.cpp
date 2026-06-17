// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Abilities/CharacterSkill/ERNGA_Ult_CrimsonJudgment.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"

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
}

void UERNGA_Ult_CrimsonJudgment::FinishSkill()
{
	if (IsActive())
	{
		K2_EndAbility();
	}
}
