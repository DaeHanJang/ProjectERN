// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GAS/ERNGameplayAbility.h"
#include "ERNGA_Flask.generated.h"

class UNiagaraSystem;

/**
 * 
 */
UCLASS()
class PROJECTERN_API UERNGA_Flask : public UERNGameplayAbility
{
	GENERATED_BODY()
	
public:
	UERNGA_Flask();
	
protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	
protected:
	UPROPERTY(EditDefaultsOnly, Category="Flask|GameplayEffect")
	TSubclassOf<UGameplayEffect> GE_Flask;
	
	UPROPERTY(EditDefaultsOnly, Category="Flask|Animation")
	TObjectPtr<UAnimMontage> DrinkMontage;
	
	UPROPERTY(EditDefaultsOnly, Category="Flask|Effect")
	TObjectPtr<UNiagaraSystem> Effect;
	
	UPROPERTY(EditDefaultsOnly, Category="Flask|Sound")
	TObjectPtr<USoundBase> Sound;
	
};
