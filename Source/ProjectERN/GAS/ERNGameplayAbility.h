// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "ERNGameplayAbility.generated.h"

class UGameplayEffect;
/**
 * 마나/스태미나 소모 등의 GE를 활용하기 위한 GA들의 공통 부모
 */
UCLASS(Abstract)
class PROJECTERN_API UERNGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()
	
public:
	UERNGameplayAbility();
	
protected:
	virtual bool CheckCost(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	virtual void ApplyCost(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) const override;

protected:
	// 스태미나 소모 값
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ERN|Cost")
	float StaminaCost = 0.f;

	// 마나 소모 값
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "ERN|Cost")
	float ManaCost = 0.f;

	// GE
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ERN|GE")
	TSubclassOf<UGameplayEffect> CostEffectClass;
	
	// 스태미나 재생 블록 GE
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ERN|GE")
	TSubclassOf<UGameplayEffect> StaminaRegenBlockEffectClass;

	// 마나 재생 블록 GE
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ERN|GE")
	TSubclassOf<UGameplayEffect> ManaRegenBlockEffectClass;
	
	// 스태미나/마나가 충분한지 확인
	bool CanPayResourceCost(float InStaminaCost, float InManaCost) const;
	
	// 스태미나/마나 소모 적용
	bool ApplyResourceCost(float InStaminaCost, float InManaCost) const;
	
	// 콤보 어택은 따로 스태미나 소모를 적용하기 위함
	bool CheckResourceCost(float InStaminaCost, float InManaCost, const FGameplayAbilityActorInfo* ActorInfo) const;
};
