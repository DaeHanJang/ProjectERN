// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GAS/Abilities/CharacterSkill/ERNGA_UltimateSkillBase.h"
#include "ERNGA_Ult_Mage.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTERN_API UERNGA_Ult_Mage : public UERNGA_UltimateSkillBase
{
	GENERATED_BODY()
	
public:
	UERNGA_Ult_Mage();
	
protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

private:
	UFUNCTION()
	void FinishSkill();
	
};
