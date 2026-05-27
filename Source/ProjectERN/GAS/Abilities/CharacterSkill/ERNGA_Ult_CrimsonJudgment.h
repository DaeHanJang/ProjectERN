// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GAS/Abilities/CharacterSkill/ERNGA_UltimateSkillBase.h"
#include "ERNGA_Ult_CrimsonJudgment.generated.h"

class AProjectERNCharacter;
class USkeletalMeshComponent;

UCLASS()
class PROJECTERN_API UERNGA_Ult_CrimsonJudgment : public UERNGA_UltimateSkillBase
{
	GENERATED_BODY()
	
public:
	UERNGA_Ult_CrimsonJudgment();

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
