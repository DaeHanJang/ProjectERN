// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Abilities/CharacterSkill/ERNGA_Ult_Mage.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"

UERNGA_Ult_Mage::UERNGA_Ult_Mage()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UERNGA_Ult_Mage::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
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

	MontageTask->OnCompleted.AddDynamic(this, &UERNGA_Ult_Mage::FinishSkill);
	MontageTask->OnBlendOut.AddDynamic(this, &UERNGA_Ult_Mage::FinishSkill);
	MontageTask->OnInterrupted.AddDynamic(this, &UERNGA_Ult_Mage::FinishSkill);
	MontageTask->OnCancelled.AddDynamic(this, &UERNGA_Ult_Mage::FinishSkill);
}

void UERNGA_Ult_Mage::FinishSkill()
{
	if (IsActive())
	{
		K2_EndAbility();
	}
}
