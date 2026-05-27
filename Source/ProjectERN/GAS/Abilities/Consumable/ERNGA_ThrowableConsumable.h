// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GAS/ERNGameplayAbility.h"
#include "ERNGA_ThrowableConsumable.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTERN_API UERNGA_ThrowableConsumable : public UERNGameplayAbility
{
	GENERATED_BODY()
	
public:
	UERNGA_ThrowableConsumable();
	
protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	
	UFUNCTION()
	void OnThrowNotifyReceived(FGameplayEventData Payload);
	
protected:
	// Animation
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Consumable|Animation", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UAnimMontage> ThrowMontage;
	
	// Projectile spawn offset from the player
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Consumable|Throw", meta=(AllowPrivateAccess="true"))
	float ThrowForwardDistance = 100.0f;
	
};
