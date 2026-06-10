// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GAS/ERNGameplayAbility.h"
#include "ERNGA_Jump.generated.h"

class UAnimMontage;
class UAbilityTask_PlayMontageAndWait;
class UAbilityTask_WaitGameplayEvent;

UCLASS()
class PROJECTERN_API UERNGA_Jump : public UERNGameplayAbility
{
	GENERATED_BODY()

public:
	UERNGA_Jump();

	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
	                                const FGameplayAbilityActorInfo* ActorInfo,
	                                const FGameplayTagContainer* SourceTags = nullptr,
	                                const FGameplayTagContainer* TargetTags = nullptr,
	                                FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	                             const FGameplayAbilityActorInfo* ActorInfo,
	                             const FGameplayAbilityActivationInfo ActivationInfo,
	                             const FGameplayEventData* TriggerEventData) override;

protected:
	/** 사용할 점프 몽타주 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Animation")
	TObjectPtr<UAnimMontage> JumpMontage;
	
	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitGameplayEvent> JumpLaunchEventTask;

	UPROPERTY()
	TObjectPtr<UAbilityTask_PlayMontageAndWait> JumpMontageTask;

	// 실제 점프가 발동했는지 확인하는 플래그
	bool bJumpLaunchConsumed = false;

	UFUNCTION()
	void OnJumpLaunchEvent(FGameplayEventData Payload);

	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnMontageInterrupted();

	UFUNCTION()
	void OnMontageCancelled();

	void FinishJumpAbility(bool bWasCancelled);
};
