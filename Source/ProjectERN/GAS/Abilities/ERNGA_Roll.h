// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GAS/ERNGameplayAbility.h"
#include "ERNGA_Roll.generated.h"

class UAnimMontage;

UCLASS()
class PROJECTERN_API UERNGA_Roll : public UERNGameplayAbility
{
	GENERATED_BODY()

public:
	UERNGA_Roll();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	                             const FGameplayAbilityActorInfo* ActorInfo,
	                             const FGameplayAbilityActivationInfo ActivationInfo,
	                             const FGameplayEventData* TriggerEventData) override;

	/*
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
	                        const FGameplayAbilityActorInfo* ActorInfo,
	                        const FGameplayAbilityActivationInfo ActivationInfo,
	                        bool bReplicateEndAbility,
	                        bool bWasCancelled) override;
	*/
protected:
	// 구르기 몽타주
	UPROPERTY(EditDefaultsOnly, Category="ERN")
	TObjectPtr<UAnimMontage> RollMontage;

	// 입력 방향에 따른 몽타주 섹션 이름 반환
	FName GetRollSection(const FVector& InputVector);
};
