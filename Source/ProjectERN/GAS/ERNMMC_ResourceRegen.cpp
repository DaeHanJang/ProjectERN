// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/ERNMMC_ResourceRegen.h"

#include "ERNAttributeSet.h"

UERNMMC_StaminaRegen::UERNMMC_StaminaRegen()
{
	// 최대 스태미나
	MaxStaminaDef = FGameplayEffectAttributeCaptureDefinition(
		UERNAttributeSet::GetMaxStaminaAttribute(),
		EGameplayEffectAttributeCaptureSource::Target,
		false);
	
	// 스태미나 재생율
	StaminaRegenRateDef = FGameplayEffectAttributeCaptureDefinition(
	UERNAttributeSet::GetStaminaRegenRateAttribute(),
	EGameplayEffectAttributeCaptureSource::Target,
	false);

	RelevantAttributesToCapture.Add(MaxStaminaDef);
	RelevantAttributesToCapture.Add(StaminaRegenRateDef);
}

float UERNMMC_StaminaRegen::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	FAggregatorEvaluateParameters EvaluationParameters;
	EvaluationParameters.SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	EvaluationParameters.TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	float MaxStamina = 0.f;
	float RegenRate = 0.f;

	GetCapturedAttributeMagnitude(MaxStaminaDef, Spec, EvaluationParameters, MaxStamina);
	GetCapturedAttributeMagnitude(StaminaRegenRateDef, Spec, EvaluationParameters, RegenRate);
	
	return MaxStamina * RegenRate;
}

UERNMMC_ManaRegen::UERNMMC_ManaRegen()
{
	MaxManaDef = FGameplayEffectAttributeCaptureDefinition(
		UERNAttributeSet::GetMaxManaAttribute(),
		EGameplayEffectAttributeCaptureSource::Target,
		false);

	ManaRegenRateDef = FGameplayEffectAttributeCaptureDefinition(
		UERNAttributeSet::GetManaRegenRateAttribute(),
		EGameplayEffectAttributeCaptureSource::Target,
		false);
	
	RelevantAttributesToCapture.Add(MaxManaDef);
	RelevantAttributesToCapture.Add(ManaRegenRateDef);
}

float UERNMMC_ManaRegen::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	FAggregatorEvaluateParameters EvaluationParameters;
	EvaluationParameters.SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	EvaluationParameters.TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	float MaxMana = 0.f;
	float RegenRate = 0.f;

	GetCapturedAttributeMagnitude(MaxManaDef, Spec, EvaluationParameters, MaxMana);
	GetCapturedAttributeMagnitude(ManaRegenRateDef, Spec, EvaluationParameters, RegenRate);
	
	return MaxMana * RegenRate;
}
