// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "ERNMMC_ResourceRegen.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTERN_API UERNMMC_StaminaRegen : public UGameplayModMagnitudeCalculation
{
	GENERATED_BODY()

public:
	UERNMMC_StaminaRegen();

	virtual float CalculateBaseMagnitude_Implementation(
		const FGameplayEffectSpec& Spec) const override;

private:
	// 어떤 Attribute를, 언제, 어떻게 가져올 것인지에 대한 정의서
	FGameplayEffectAttributeCaptureDefinition MaxStaminaDef;
};

UCLASS()
class PROJECTERN_API UERNMMC_ManaRegen : public UGameplayModMagnitudeCalculation
{
	GENERATED_BODY()

public:
	UERNMMC_ManaRegen();

	virtual float CalculateBaseMagnitude_Implementation(
		const FGameplayEffectSpec& Spec) const override;

private:
	FGameplayEffectAttributeCaptureDefinition MaxManaDef;
};