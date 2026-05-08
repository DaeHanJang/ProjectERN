// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/ERNGameplayAbility.h"

#include "ERNAttributeSet.h"
#include "ERNGameplayTags.h"

UERNGameplayAbility::UERNGameplayAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

bool UERNGameplayAbility::CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
                                    FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CheckCost(Handle, ActorInfo, OptionalRelevantTags))
	{
		return false;
	}

	if (!ActorInfo || !ActorInfo->AbilitySystemComponent.IsValid())
	{
		return false;
	}

	const UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();

	if (StaminaCost > 0.f)
	{
		const float CurrentStamina = ASC->GetNumericAttribute(UERNAttributeSet::GetStaminaAttribute());
		if (CurrentStamina < StaminaCost)
		{
			return false;
		}
	}

	if (ManaCost > 0.f)
	{
		const float CurrentMana = ASC->GetNumericAttribute(UERNAttributeSet::GetManaAttribute());
		if (CurrentMana < ManaCost)
		{
			return false;
		}
	}

	return true;
}

void UERNGameplayAbility::ApplyCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo) const
{
	Super::ApplyCost(Handle, ActorInfo, ActivationInfo);

	if (!ActorInfo || !ActorInfo->AbilitySystemComponent.IsValid())
	{
		return;
	}
	
	if (!CostEffectClass || (StaminaCost <= 0.f && ManaCost <= 0.f))
	{
		return;
	}

	// Actor의 ASC정보를 가져옴
	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();

	/*
	FGameplayEffectContextHandle ContextHandle = ASC->MakeEffectContext();
	ContextHandle.AddSourceObject(this);

	FGameplayEffectSpecHandle SpecHandle =
		ASC->MakeOutgoingSpec(CostEffectClass, GetAbilityLevel(Handle, ActorInfo), ContextHandle);

	if (!SpecHandle.IsValid())
	{
		return;
	}

	SpecHandle.Data->SetSetByCallerMagnitude(TAG_Data_Cost_Stamina, -StaminaCost);
	SpecHandle.Data->SetSetByCallerMagnitude(TAG_Data_Cost_Mana, -ManaCost);

	ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	*/
	
	FGameplayEffectContextHandle ContextHandle = ASC->MakeEffectContext();
	ContextHandle.AddSourceObject(this);

	if (CostEffectClass)
	{
		FGameplayEffectSpecHandle CostSpecHandle =
			ASC->MakeOutgoingSpec(
				CostEffectClass,
				GetAbilityLevel(Handle, ActorInfo),
				ContextHandle);

		if (CostSpecHandle.IsValid())
		{
			CostSpecHandle.Data->SetSetByCallerMagnitude(
				TAG_Data_Cost_Stamina,
				-StaminaCost);

			CostSpecHandle.Data->SetSetByCallerMagnitude(
				TAG_Data_Cost_Mana,
				-ManaCost);

			ASC->ApplyGameplayEffectSpecToSelf(*CostSpecHandle.Data.Get());
		}
	}

	if (StaminaCost > 0.f && StaminaRegenBlockEffectClass)
	{
		FGameplayEffectSpecHandle BlockSpecHandle =
			ASC->MakeOutgoingSpec(
				StaminaRegenBlockEffectClass,
				GetAbilityLevel(Handle, ActorInfo),
				ContextHandle);

		if (BlockSpecHandle.IsValid())
		{
			ASC->ApplyGameplayEffectSpecToSelf(*BlockSpecHandle.Data.Get());
		}
	}

	if (ManaCost > 0.f && ManaRegenBlockEffectClass)
	{
		FGameplayEffectSpecHandle BlockSpecHandle =
			ASC->MakeOutgoingSpec(
				ManaRegenBlockEffectClass,
				GetAbilityLevel(Handle, ActorInfo),
				ContextHandle);

		if (BlockSpecHandle.IsValid())
		{
			ASC->ApplyGameplayEffectSpecToSelf(*BlockSpecHandle.Data.Get());
		}
	}
}
