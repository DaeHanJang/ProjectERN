// Copyright Epic Games, Inc. All Rights Reserved.

#include "GAS/ERNAttributeSet.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffect.h"
#include "GameplayEffectExtension.h"
#include "Character/Player/ProjectERNCharacter.h"
#include "GAS/ERNGameplayTags.h"

UERNAttributeSet::UERNAttributeSet()
{
	// 기본값 설정
	InitHealth(100.f);
	InitMaxHealth(100.f);
	InitMana(100.f);
	InitMaxMana(100.f);
	InitManaRegenRate(0.01f);
	InitStamina(100.f);
	InitMaxStamina(100.f);
	InitStaminaRegenRate(0.03f);
	InitAttackPower(10.f);
	InitDefense(5.f);
	InitMoveSpeed(600.f);
	InitLevel(1.f);
	InitGold(0.f);
	InitStaggerResistance(10.f);
	InitDownResistance(20.f);
	InitMaxFlaskQuantity(7.0f);
	InitFlaskQuantity(7.0f);
	InitShield(0.f);
}

void UERNAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UERNAttributeSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UERNAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UERNAttributeSet, Mana, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UERNAttributeSet, MaxMana, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UERNAttributeSet, ManaRegenRate, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UERNAttributeSet, Stamina, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UERNAttributeSet, MaxStamina, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UERNAttributeSet, StaminaRegenRate, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UERNAttributeSet, AttackPower, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UERNAttributeSet, AttackPowerBonus, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UERNAttributeSet, Defense, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UERNAttributeSet, MoveSpeed, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UERNAttributeSet, Level, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UERNAttributeSet, Gold, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UERNAttributeSet, StaggerResistance, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UERNAttributeSet, DownResistance, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UERNAttributeSet, MaxFlaskQuantity, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UERNAttributeSet, FlaskQuantity, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UERNAttributeSet, Shield, COND_None, REPNOTIFY_Always);
}

void UERNAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);
	
	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxHealth());
	}
	else if (Attribute == GetMaxHealthAttribute())
	{
		// 최대 체력이 0 이하로 내려가는 것을 방지 (최소 10 유지)
		NewValue = FMath::Max(NewValue, 10.f);
	}
	else if (Attribute == GetManaAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxMana());
	}
	else if (Attribute == GetStaminaAttribute())
	{
		// OutOfCombat 시 감소 차단 (재생/회복은 통과)
		if (NewValue < GetStamina())
		{
			if (UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent())
			{
				if (ASC->HasMatchingGameplayTag(TAG_State_OutOfCombat))
				{
					NewValue = GetStamina();
					return;
				}
			}
		}
		NewValue = FMath::Clamp(NewValue, 0.f, GetMaxStamina());
	}
	else if (Attribute == GetShieldAttribute())
	{
		// No Max for Shield
		NewValue = FMath::Max(0.f, NewValue);
	}
}

void UERNAttributeSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);

	// Max 값이 변경되었을 때, 현재 값이 Max를 초과하면 Clamp 처리
	// 또한 Max 값이 증가했다면 증가분만큼 현재 값도 회복시켜줌 (아이템 장착 시 보너스 등)
	if (Attribute == GetMaxHealthAttribute())
	{
		if (NewValue > OldValue)
		{
			if (UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent())
			{
				ASC->ApplyModToAttributeUnsafe(GetHealthAttribute(), EGameplayModOp::Additive, NewValue - OldValue);
			}
		}

		if (GetHealth() > NewValue)
		{
			if (UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent())
			{
				ASC->ApplyModToAttributeUnsafe(GetHealthAttribute(), EGameplayModOp::Override, NewValue);
			}
		}
	}
	else if (Attribute == GetMaxManaAttribute())
	{
		if (NewValue > OldValue)
		{
			if (UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent())
			{
				ASC->ApplyModToAttributeUnsafe(GetManaAttribute(), EGameplayModOp::Additive, NewValue - OldValue);
			}
		}

		if (GetMana() > NewValue)
		{
			if (UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent())
			{
				ASC->ApplyModToAttributeUnsafe(GetManaAttribute(), EGameplayModOp::Override, NewValue);
			}
		}
	}
	else if (Attribute == GetMaxStaminaAttribute())
	{
		if (NewValue > OldValue)
		{
			if (UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent())
			{
				ASC->ApplyModToAttributeUnsafe(GetStaminaAttribute(), EGameplayModOp::Additive, NewValue - OldValue);
			}
		}

		if (GetStamina() > NewValue)
		{
			if (UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent())
			{
				ASC->ApplyModToAttributeUnsafe(GetStaminaAttribute(), EGameplayModOp::Override, NewValue);
			}
		}
	}
}

void UERNAttributeSet::PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const
{
	Super::PreAttributeBaseChange(Attribute, NewValue);

	// 인스턴트 GE로 Stamina Base 감소 시 OutOfCombat이면 차단
	if (Attribute == GetStaminaAttribute() && NewValue < GetStamina())
	{
		if (UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent())
		{
			if (ASC->HasMatchingGameplayTag(TAG_State_OutOfCombat))
			{
				NewValue = GetStamina();
			}
		}
	}
	else if (Attribute == GetMaxHealthAttribute())
	{
		NewValue = FMath::Max(NewValue, 10.f);
	}
}

void UERNAttributeSet::PostGameplayEffectExecute(const struct FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		float MinHealth = 0.f;
		if (const AProjectERNCharacter* Player = Cast<AProjectERNCharacter>(Data.Target.GetAvatarActor()))
		{
			if (Player->bGodMode)
			{
				MinHealth = 1.f;
			}
		}
		SetHealth(FMath::Clamp(GetHealth(), MinHealth, GetMaxHealth()));
	}
	else if (Data.EvaluatedData.Attribute == GetManaAttribute())
	{
		SetMana(FMath::Clamp(GetMana(), 0.f, GetMaxMana()));
	}
	else if (Data.EvaluatedData.Attribute == GetStaminaAttribute())
	{
		SetStamina(FMath::Clamp(GetStamina(), 0.f, GetMaxStamina()));
	}
	else if (Data.EvaluatedData.Attribute == GetShieldAttribute())
	{
		SetShield(FMath::Max(0.f, GetShield()));
	}
}

void UERNAttributeSet::OnRep_Health(const FGameplayAttributeData& OldHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UERNAttributeSet, Health, OldHealth);
}

void UERNAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UERNAttributeSet, MaxHealth, OldMaxHealth);
}

void UERNAttributeSet::OnRep_Mana(const FGameplayAttributeData& OldMana)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UERNAttributeSet, Mana, OldMana);
}

void UERNAttributeSet::OnRep_MaxMana(const FGameplayAttributeData& OldMaxMana)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UERNAttributeSet, MaxMana, OldMaxMana);
}

void UERNAttributeSet::OnRep_ManaRegenRate(const FGameplayAttributeData& OldManaRegenRate)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UERNAttributeSet, ManaRegenRate, OldManaRegenRate);
}

void UERNAttributeSet::OnRep_Stamina(const FGameplayAttributeData& OldStamina)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UERNAttributeSet, Stamina, OldStamina);
}

void UERNAttributeSet::OnRep_MaxStamina(const FGameplayAttributeData& OldMaxStamina)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UERNAttributeSet, MaxStamina, OldMaxStamina);
}

void UERNAttributeSet::OnRep_StaminaRegenRate(const FGameplayAttributeData& OldStaminaRegenRate)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UERNAttributeSet, StaminaRegenRate, OldStaminaRegenRate);
}

void UERNAttributeSet::OnRep_AttackPower(const FGameplayAttributeData& OldAttackPower)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UERNAttributeSet, AttackPower, OldAttackPower);
}

void UERNAttributeSet::OnRep_AttackPowerBonus(const FGameplayAttributeData& OldAttackPowerBonus)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UERNAttributeSet, AttackPowerBonus, OldAttackPowerBonus);
}

void UERNAttributeSet::OnRep_Defense(const FGameplayAttributeData& OldDefense)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UERNAttributeSet, Defense, OldDefense);
}

void UERNAttributeSet::OnRep_MoveSpeed(const FGameplayAttributeData& OldMoveSpeed)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UERNAttributeSet, MoveSpeed, OldMoveSpeed);
}

void UERNAttributeSet::OnRep_Level(const FGameplayAttributeData& OldLevel)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UERNAttributeSet, Level, OldLevel);
}

void UERNAttributeSet::OnRep_Gold(const FGameplayAttributeData& OldGold)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UERNAttributeSet, Gold, OldGold);
}

void UERNAttributeSet::OnRep_StaggerResistance(const FGameplayAttributeData& OldStaggerResistance)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UERNAttributeSet, StaggerResistance, OldStaggerResistance);
}

void UERNAttributeSet::OnRep_DownResistance(const FGameplayAttributeData& OldDownResistance)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UERNAttributeSet, DownResistance, OldDownResistance);
}

void UERNAttributeSet::OnRep_MaxFlaskQuantity(const FGameplayAttributeData& OldMaxFlaskQuantity)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UERNAttributeSet, MaxFlaskQuantity, OldMaxFlaskQuantity);
}

void UERNAttributeSet::OnRep_FlaskQuantity(const FGameplayAttributeData& OldFlaskQuantity)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UERNAttributeSet, FlaskQuantity, OldFlaskQuantity);
}

void UERNAttributeSet::OnRep_Shield(const FGameplayAttributeData& OldShield)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UERNAttributeSet, Shield, OldShield);

}
