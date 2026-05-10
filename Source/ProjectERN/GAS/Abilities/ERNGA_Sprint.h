// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GAS/ERNGameplayAbility.h"
#include "ERNGA_Sprint.generated.h"

class UGameplayEffect;
struct FOnAttributeChangeData;

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
	
protected:
	// 초당 소모될 Stamina양
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ERN|Cost")
	float StaminaCostPerSecond = 10.f;

	// GE 실행 주기
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ERN|Cost")
	float StaminaDrainPeriod = 0.1f;

	// 설정할 GE클래스
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ERN|GE")
	TSubclassOf<UGameplayEffect> SprintStaminaDrainEffectClass;

	FActiveGameplayEffectHandle StaminaDrainEffectHandle;

	void ApplyStaminaDrainEffect(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo);

	void RemoveStaminaDrainEffect();

	void HandleStaminaChanged(const FOnAttributeChangeData& Data);
	
};
