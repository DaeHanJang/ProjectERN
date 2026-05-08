// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/ERNMMC_ResourceRegen.h"

#include "ERNAttributeSet.h"

UERNMMC_StaminaRegen::UERNMMC_StaminaRegen()
{
	MaxStaminaDef = FGameplayEffectAttributeCaptureDefinition(
		UERNAttributeSet::GetMaxStaminaAttribute(),
		EGameplayEffectAttributeCaptureSource::Target,
		false);

	RelevantAttributesToCapture.Add(MaxStaminaDef);
}

float UERNMMC_StaminaRegen::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	FAggregatorEvaluateParameters EvaluationParameters;
	EvaluationParameters.SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	EvaluationParameters.TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	float MaxStamina = 0.f;

	GetCapturedAttributeMagnitude(
		MaxStaminaDef,
		Spec,
		EvaluationParameters,
		MaxStamina);

	constexpr float RegenPercentPerTick = 0.03f;
	return MaxStamina * RegenPercentPerTick;
}

UERNMMC_ManaRegen::UERNMMC_ManaRegen()
{
	MaxManaDef = FGameplayEffectAttributeCaptureDefinition(
		UERNAttributeSet::GetMaxManaAttribute(),
		EGameplayEffectAttributeCaptureSource::Target,
		false);

	RelevantAttributesToCapture.Add(MaxManaDef);
}

float UERNMMC_ManaRegen::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	FAggregatorEvaluateParameters EvaluationParameters;
	EvaluationParameters.SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	EvaluationParameters.TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	float MaxMana = 0.f;

	GetCapturedAttributeMagnitude(
		MaxManaDef,
		Spec,
		EvaluationParameters,
		MaxMana);

	constexpr float RegenPercentPerTick = 0.03f;
	return MaxMana * RegenPercentPerTick;
}
