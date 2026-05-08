// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GAS/ERNGameplayAbility.h"
#include "ERNGA_AttackAbility.generated.h"

/**
 * UERNA_AttackAbility - 공격 어빌리티 베이스
 * 태그 관리만 C++에서 처리
 * 몽타주 재생 및 EndAbility는 블루프린트에서 처리
 */
UCLASS(Blueprintable)
class PROJECTERN_API UERNA_AttackAbility : public UERNGameplayAbility
{
	GENERATED_BODY()

public:
	UERNA_AttackAbility();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	                             const FGameplayAbilityActorInfo* ActorInfo,
	                             const FGameplayAbilityActivationInfo ActivationInfo,
	                             const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	                        const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility,
	                        bool bWasCancelled) override;

	// 몽타주 프로퍼티 (BP에서 편집 가능)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Animation")
	TObjectPtr<UAnimMontage> AttackMontage;
};
