// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GAS/ERNGameplayAbility.h"
#include "ERNGA_DrinkableConsumable.generated.h"

class UNiagaraSystem;

/**
 * 
 */
UCLASS()
class PROJECTERN_API UERNGA_DrinkableConsumable : public UERNGameplayAbility
{
	GENERATED_BODY()
	
public:
	UERNGA_DrinkableConsumable();
	
protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	
protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Consumable|GameplayEffect", meta=(AllowPrivateAccess="true"))
	TSubclassOf<UGameplayEffect> GE_DrinkableConsumable;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Consumable|Animation", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UAnimMontage> DrinkMontage;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Consumable|Effect", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UNiagaraSystem> Effect;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Consumable|Sound", meta=(AllowPrivateAccess="true"))
	TObjectPtr<USoundBase> Sound;
	
};
