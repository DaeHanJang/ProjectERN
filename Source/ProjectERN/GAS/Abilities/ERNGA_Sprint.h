// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GAS/ERNGameplayAbility.h"
#include "ERNGA_Sprint.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTERN_API UERNGA_Sprint : public UERNGameplayAbility
{
	GENERATED_BODY()

public:
	UERNGA_Sprint();

	// Sprint 시작 몽타주
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Animation")
	TObjectPtr<UAnimMontage> SprintStartMontage;

	// Sprint 끝 몽타주
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Animation")
	TObjectPtr<UAnimMontage> SprintEndMontage;
	
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	                             const FGameplayAbilityActorInfo* ActorInfo,
	                             const FGameplayAbilityActivationInfo ActivationInfo,
	                             const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
	                        const FGameplayAbilityActorInfo* ActorInfo,
	                        const FGameplayAbilityActivationInfo ActivationInfo,
	                        bool bReplicateEndAbility,
	                        bool bWasCancelled) override;
	
	// 끝 요청
	void RequestStopSprint();
};
