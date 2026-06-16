#include "UI/ERNPlayerDetailStatusWidget.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GAS/ERNAttributeSet.h"
#include "GameFramework/PlayerController.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "Character/Player/ProjectERNCharacter.h"
#include "Inventory/Components/ERNEquipmentComponent.h"
#include "Combat/Weapons/ERNWeaponBase.h"

void UERNPlayerDetailStatusWidget::NativeConstruct()
{
	Super::NativeConstruct();
	TryInitASC();
}

bool UERNPlayerDetailStatusWidget::TryInitASC()
{
	if (CachedASC.IsValid()) return true;

	if (APlayerController* PC = GetOwningPlayer())
	{
		if (APawn* Pawn = PC->GetPawn())
		{
			CachedASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Pawn);
			if (CachedASC.IsValid())
			{
				// 기존에 바인딩된 게 있다면 해제
				NativeDestruct();

				// 능력치 변경 이벤트 바인딩
				HealthChangedDelegateHandle = CachedASC->GetGameplayAttributeValueChangeDelegate(UERNAttributeSet::GetHealthAttribute()).AddUObject(this, &UERNPlayerDetailStatusWidget::HealthChanged);
				MaxHealthChangedDelegateHandle = CachedASC->GetGameplayAttributeValueChangeDelegate(UERNAttributeSet::GetMaxHealthAttribute()).AddUObject(this, &UERNPlayerDetailStatusWidget::MaxHealthChanged);
				
				ManaChangedDelegateHandle = CachedASC->GetGameplayAttributeValueChangeDelegate(UERNAttributeSet::GetManaAttribute()).AddUObject(this, &UERNPlayerDetailStatusWidget::ManaChanged);
				MaxManaChangedDelegateHandle = CachedASC->GetGameplayAttributeValueChangeDelegate(UERNAttributeSet::GetMaxManaAttribute()).AddUObject(this, &UERNPlayerDetailStatusWidget::MaxManaChanged);
				
				StaminaChangedDelegateHandle = CachedASC->GetGameplayAttributeValueChangeDelegate(UERNAttributeSet::GetStaminaAttribute()).AddUObject(this, &UERNPlayerDetailStatusWidget::StaminaChanged);
				MaxStaminaChangedDelegateHandle = CachedASC->GetGameplayAttributeValueChangeDelegate(UERNAttributeSet::GetMaxStaminaAttribute()).AddUObject(this, &UERNPlayerDetailStatusWidget::MaxStaminaChanged);
				
				AttackPowerChangedDelegateHandle = CachedASC->GetGameplayAttributeValueChangeDelegate(UERNAttributeSet::GetAttackPowerAttribute()).AddUObject(this, &UERNPlayerDetailStatusWidget::AttackPowerChanged);
				AttackPowerBonusChangedDelegateHandle = CachedASC->GetGameplayAttributeValueChangeDelegate(UERNAttributeSet::GetAttackPowerBonusAttribute()).AddUObject(this, &UERNPlayerDetailStatusWidget::AttackPowerBonusChanged);
				
				DefenseChangedDelegateHandle = CachedASC->GetGameplayAttributeValueChangeDelegate(UERNAttributeSet::GetDefenseAttribute()).AddUObject(this, &UERNPlayerDetailStatusWidget::DefenseChanged);
				MoveSpeedChangedDelegateHandle = CachedASC->GetGameplayAttributeValueChangeDelegate(UERNAttributeSet::GetMoveSpeedAttribute()).AddUObject(this, &UERNPlayerDetailStatusWidget::MoveSpeedChanged);
				LevelChangedDelegateHandle = CachedASC->GetGameplayAttributeValueChangeDelegate(UERNAttributeSet::GetLevelAttribute()).AddUObject(this, &UERNPlayerDetailStatusWidget::LevelChanged);
				GoldChangedDelegateHandle = CachedASC->GetGameplayAttributeValueChangeDelegate(UERNAttributeSet::GetGoldAttribute()).AddUObject(this, &UERNPlayerDetailStatusWidget::GoldChanged);
				StaggerResistChangedDelegateHandle = CachedASC->GetGameplayAttributeValueChangeDelegate(UERNAttributeSet::GetStaggerResistanceAttribute()).AddUObject(this, &UERNPlayerDetailStatusWidget::StaggerResistChanged);
				DownResistChangedDelegateHandle = CachedASC->GetGameplayAttributeValueChangeDelegate(UERNAttributeSet::GetDownResistanceAttribute()).AddUObject(this, &UERNPlayerDetailStatusWidget::DownResistChanged);
				FlaskChangedDelegateHandle = CachedASC->GetGameplayAttributeValueChangeDelegate(UERNAttributeSet::GetFlaskQuantityAttribute()).AddUObject(this, &UERNPlayerDetailStatusWidget::FlaskChanged);
				MaxFlaskChangedDelegateHandle = CachedASC->GetGameplayAttributeValueChangeDelegate(UERNAttributeSet::GetMaxFlaskQuantityAttribute()).AddUObject(this, &UERNPlayerDetailStatusWidget::MaxFlaskChanged);
				ShieldChangedDelegateHandle = CachedASC->GetGameplayAttributeValueChangeDelegate(UERNAttributeSet::GetShieldAttribute()).AddUObject(this, &UERNPlayerDetailStatusWidget::ShieldChanged);

				// UI가 켜질 때 현재 수치로 한 번 강제 갱신
				RefreshAllAttributes();
				
				if (AProjectERNCharacter* Character = Cast<AProjectERNCharacter>(Pawn))
				{
					if (UERNEquipmentComponent* EquipComp = Character->GetEquipmentComponent())
					{
						EquipComp->OnEquipmentSlotChanged.AddUniqueDynamic(this, &UERNPlayerDetailStatusWidget::OnEquipmentSlotChanged);
					}
				}
				
				return true;
			}
		}
	}
	return false;
}

void UERNPlayerDetailStatusWidget::NativeDestruct()
{
	if (CachedASC.IsValid())
	{
		CachedASC->GetGameplayAttributeValueChangeDelegate(UERNAttributeSet::GetHealthAttribute()).Remove(HealthChangedDelegateHandle);
		CachedASC->GetGameplayAttributeValueChangeDelegate(UERNAttributeSet::GetMaxHealthAttribute()).Remove(MaxHealthChangedDelegateHandle);
		CachedASC->GetGameplayAttributeValueChangeDelegate(UERNAttributeSet::GetManaAttribute()).Remove(ManaChangedDelegateHandle);
		CachedASC->GetGameplayAttributeValueChangeDelegate(UERNAttributeSet::GetMaxManaAttribute()).Remove(MaxManaChangedDelegateHandle);
		CachedASC->GetGameplayAttributeValueChangeDelegate(UERNAttributeSet::GetStaminaAttribute()).Remove(StaminaChangedDelegateHandle);
		CachedASC->GetGameplayAttributeValueChangeDelegate(UERNAttributeSet::GetMaxStaminaAttribute()).Remove(MaxStaminaChangedDelegateHandle);
		CachedASC->GetGameplayAttributeValueChangeDelegate(UERNAttributeSet::GetAttackPowerAttribute()).Remove(AttackPowerChangedDelegateHandle);
		CachedASC->GetGameplayAttributeValueChangeDelegate(UERNAttributeSet::GetAttackPowerBonusAttribute()).Remove(AttackPowerBonusChangedDelegateHandle);
		CachedASC->GetGameplayAttributeValueChangeDelegate(UERNAttributeSet::GetDefenseAttribute()).Remove(DefenseChangedDelegateHandle);
		CachedASC->GetGameplayAttributeValueChangeDelegate(UERNAttributeSet::GetMoveSpeedAttribute()).Remove(MoveSpeedChangedDelegateHandle);
		CachedASC->GetGameplayAttributeValueChangeDelegate(UERNAttributeSet::GetLevelAttribute()).Remove(LevelChangedDelegateHandle);
		CachedASC->GetGameplayAttributeValueChangeDelegate(UERNAttributeSet::GetGoldAttribute()).Remove(GoldChangedDelegateHandle);
		CachedASC->GetGameplayAttributeValueChangeDelegate(UERNAttributeSet::GetStaggerResistanceAttribute()).Remove(StaggerResistChangedDelegateHandle);
		CachedASC->GetGameplayAttributeValueChangeDelegate(UERNAttributeSet::GetDownResistanceAttribute()).Remove(DownResistChangedDelegateHandle);
		CachedASC->GetGameplayAttributeValueChangeDelegate(UERNAttributeSet::GetFlaskQuantityAttribute()).Remove(FlaskChangedDelegateHandle);
		CachedASC->GetGameplayAttributeValueChangeDelegate(UERNAttributeSet::GetMaxFlaskQuantityAttribute()).Remove(MaxFlaskChangedDelegateHandle);
		CachedASC->GetGameplayAttributeValueChangeDelegate(UERNAttributeSet::GetShieldAttribute()).Remove(ShieldChangedDelegateHandle);
	}

	if (APlayerController* PC = GetOwningPlayer())
	{
		if (APawn* Pawn = PC->GetPawn())
		{
			if (AProjectERNCharacter* Character = Cast<AProjectERNCharacter>(Pawn))
			{
				if (UERNEquipmentComponent* EquipComp = Character->GetEquipmentComponent())
				{
					EquipComp->OnEquipmentSlotChanged.RemoveDynamic(this, &UERNPlayerDetailStatusWidget::OnEquipmentSlotChanged);
				}
			}
		}
	}

	Super::NativeDestruct();
}

void UERNPlayerDetailStatusWidget::RefreshAllAttributes()
{
	if (!CachedASC.IsValid())
	{
		if (!TryInitASC()) return;
	}

	bool bFound = false;
	
	UpdateLevelText(CachedASC->GetGameplayAttributeValue(UERNAttributeSet::GetLevelAttribute(), bFound));
	UpdateGoldText(CachedASC->GetGameplayAttributeValue(UERNAttributeSet::GetGoldAttribute(), bFound));
	UpdateDefenseText(CachedASC->GetGameplayAttributeValue(UERNAttributeSet::GetDefenseAttribute(), bFound));
	UpdateMoveSpeedText(CachedASC->GetGameplayAttributeValue(UERNAttributeSet::GetMoveSpeedAttribute(), bFound));
	UpdateStaggerResistText(CachedASC->GetGameplayAttributeValue(UERNAttributeSet::GetStaggerResistanceAttribute(), bFound));
	UpdateDownResistText(CachedASC->GetGameplayAttributeValue(UERNAttributeSet::GetDownResistanceAttribute(), bFound));
	UpdateShieldText(CachedASC->GetGameplayAttributeValue(UERNAttributeSet::GetShieldAttribute(), bFound));

	float CurrentHealth = CachedASC->GetGameplayAttributeValue(UERNAttributeSet::GetHealthAttribute(), bFound);
	float MaxHealth = CachedASC->GetGameplayAttributeValue(UERNAttributeSet::GetMaxHealthAttribute(), bFound);
	UpdateHealthText(CurrentHealth, MaxHealth);

	float CurrentMana = CachedASC->GetGameplayAttributeValue(UERNAttributeSet::GetManaAttribute(), bFound);
	float MaxMana = CachedASC->GetGameplayAttributeValue(UERNAttributeSet::GetMaxManaAttribute(), bFound);
	UpdateManaText(CurrentMana, MaxMana);

	float CurrentStamina = CachedASC->GetGameplayAttributeValue(UERNAttributeSet::GetStaminaAttribute(), bFound);
	float MaxStamina = CachedASC->GetGameplayAttributeValue(UERNAttributeSet::GetMaxStaminaAttribute(), bFound);
	UpdateStaminaText(CurrentStamina, MaxStamina);

	float CurrentFlask = CachedASC->GetGameplayAttributeValue(UERNAttributeSet::GetFlaskQuantityAttribute(), bFound);
	float MaxFlask = CachedASC->GetGameplayAttributeValue(UERNAttributeSet::GetMaxFlaskQuantityAttribute(), bFound);
	UpdateFlaskText(CurrentFlask, MaxFlask);

	float BaseAttack = CachedASC->GetGameplayAttributeValue(UERNAttributeSet::GetAttackPowerAttribute(), bFound);
	float BonusAttack = CachedASC->GetGameplayAttributeValue(UERNAttributeSet::GetAttackPowerBonusAttribute(), bFound);
	UpdateAttackPowerText(BaseAttack, BonusAttack);
}

void UERNPlayerDetailStatusWidget::HealthChanged(const FOnAttributeChangeData& Data)
{
	if (CachedASC.IsValid())
	{
		bool bFound = false;
		float Max = CachedASC->GetGameplayAttributeValue(UERNAttributeSet::GetMaxHealthAttribute(), bFound);
		UpdateHealthText(Data.NewValue, Max);
	}
}

void UERNPlayerDetailStatusWidget::MaxHealthChanged(const FOnAttributeChangeData& Data)
{
	if (CachedASC.IsValid())
	{
		bool bFound = false;
		float Current = CachedASC->GetGameplayAttributeValue(UERNAttributeSet::GetHealthAttribute(), bFound);
		UpdateHealthText(Current, Data.NewValue);
	}
}

void UERNPlayerDetailStatusWidget::ManaChanged(const FOnAttributeChangeData& Data)
{
	if (CachedASC.IsValid())
	{
		bool bFound = false;
		float Max = CachedASC->GetGameplayAttributeValue(UERNAttributeSet::GetMaxManaAttribute(), bFound);
		UpdateManaText(Data.NewValue, Max);
	}
}

void UERNPlayerDetailStatusWidget::MaxManaChanged(const FOnAttributeChangeData& Data)
{
	if (CachedASC.IsValid())
	{
		bool bFound = false;
		float Current = CachedASC->GetGameplayAttributeValue(UERNAttributeSet::GetManaAttribute(), bFound);
		UpdateManaText(Current, Data.NewValue);
	}
}

void UERNPlayerDetailStatusWidget::StaminaChanged(const FOnAttributeChangeData& Data)
{
	if (CachedASC.IsValid())
	{
		bool bFound = false;
		float Max = CachedASC->GetGameplayAttributeValue(UERNAttributeSet::GetMaxStaminaAttribute(), bFound);
		UpdateStaminaText(Data.NewValue, Max);
	}
}

void UERNPlayerDetailStatusWidget::MaxStaminaChanged(const FOnAttributeChangeData& Data)
{
	if (CachedASC.IsValid())
	{
		bool bFound = false;
		float Current = CachedASC->GetGameplayAttributeValue(UERNAttributeSet::GetStaminaAttribute(), bFound);
		UpdateStaminaText(Current, Data.NewValue);
	}
}

void UERNPlayerDetailStatusWidget::FlaskChanged(const FOnAttributeChangeData& Data)
{
	if (CachedASC.IsValid())
	{
		bool bFound = false;
		float Max = CachedASC->GetGameplayAttributeValue(UERNAttributeSet::GetMaxFlaskQuantityAttribute(), bFound);
		UpdateFlaskText(Data.NewValue, Max);
	}
}

void UERNPlayerDetailStatusWidget::MaxFlaskChanged(const FOnAttributeChangeData& Data)
{
	if (CachedASC.IsValid())
	{
		bool bFound = false;
		float Current = CachedASC->GetGameplayAttributeValue(UERNAttributeSet::GetFlaskQuantityAttribute(), bFound);
		UpdateFlaskText(Current, Data.NewValue);
	}
}

void UERNPlayerDetailStatusWidget::AttackPowerChanged(const FOnAttributeChangeData& Data)
{
	if (CachedASC.IsValid())
	{
		bool bFound = false;
		float Bonus = CachedASC->GetGameplayAttributeValue(UERNAttributeSet::GetAttackPowerBonusAttribute(), bFound);
		UpdateAttackPowerText(Data.NewValue, Bonus);
	}
}

void UERNPlayerDetailStatusWidget::AttackPowerBonusChanged(const FOnAttributeChangeData& Data)
{
	if (CachedASC.IsValid())
	{
		bool bFound = false;
		float Base = CachedASC->GetGameplayAttributeValue(UERNAttributeSet::GetAttackPowerAttribute(), bFound);
		UpdateAttackPowerText(Base, Data.NewValue);
	}
}

void UERNPlayerDetailStatusWidget::DefenseChanged(const FOnAttributeChangeData& Data)
{
	UpdateDefenseText(Data.NewValue);
}

void UERNPlayerDetailStatusWidget::MoveSpeedChanged(const FOnAttributeChangeData& Data)
{
	UpdateMoveSpeedText(Data.NewValue);
}

void UERNPlayerDetailStatusWidget::LevelChanged(const FOnAttributeChangeData& Data)
{
	UpdateLevelText(Data.NewValue);
}

void UERNPlayerDetailStatusWidget::GoldChanged(const FOnAttributeChangeData& Data)
{
	UpdateGoldText(Data.NewValue);
}

void UERNPlayerDetailStatusWidget::StaggerResistChanged(const FOnAttributeChangeData& Data)
{
	UpdateStaggerResistText(Data.NewValue);
}

void UERNPlayerDetailStatusWidget::DownResistChanged(const FOnAttributeChangeData& Data)
{
	UpdateDownResistText(Data.NewValue);
}

void UERNPlayerDetailStatusWidget::ShieldChanged(const FOnAttributeChangeData& Data)
{
	UpdateShieldText(Data.NewValue);
}

// ----------------------------------------------------
// Text Update Helpers
// ----------------------------------------------------

void UERNPlayerDetailStatusWidget::UpdateLevelText(float Level)
{
	if (Text_Level) Text_Level->SetText(FText::AsNumber(FMath::RoundToInt(Level)));
}

void UERNPlayerDetailStatusWidget::UpdateGoldText(float Gold)
{
	if (Text_Gold) Text_Gold->SetText(FText::AsNumber(FMath::RoundToInt(Gold)));
}

void UERNPlayerDetailStatusWidget::UpdateAttackPowerText(float Base, float Bonus)
{
	float WeaponDamage = GetWeaponDamage();
	float GEBonus = 0.0f;
	if (CachedASC.IsValid())
	{
		float BaseAttr = CachedASC->GetNumericAttributeBase(UERNAttributeSet::GetAttackPowerAttribute());
		GEBonus = Base - BaseAttr;
		Base = BaseAttr;
	}

	float TotalBonus = Bonus + WeaponDamage + GEBonus;

	if (Text_AttackPower)
	{
		if (TotalBonus != 0.0f)
		{
			FText FormattedText = FText::Format(NSLOCTEXT("Status", "AttackPowerBonus", "{0} ({1}{2})"), 
				FText::AsNumber(FMath::RoundToInt(Base + TotalBonus)), 
				TotalBonus > 0 ? FText::FromString(TEXT("+")) : FText::GetEmpty(),
				FText::AsNumber(FMath::RoundToInt(TotalBonus)));
			Text_AttackPower->SetText(FormattedText);
		}
		else
		{
			Text_AttackPower->SetText(FText::AsNumber(FMath::RoundToInt(Base)));
		}
	}
}

void UERNPlayerDetailStatusWidget::UpdateDefenseText(float Defense)
{
	if (Text_Defense) 
	{
		float Base = Defense;
		float Bonus = 0.0f;
		if (CachedASC.IsValid())
		{
			Base = CachedASC->GetNumericAttributeBase(UERNAttributeSet::GetDefenseAttribute());
			Bonus = Defense - Base;
		}

		if (Bonus != 0.0f)
		{
			Text_Defense->SetText(FText::Format(NSLOCTEXT("Status", "DefenseBonus", "{0} ({1}{2})"), 
				FText::AsNumber(FMath::RoundToInt(Defense)), 
				Bonus > 0 ? FText::FromString(TEXT("+")) : FText::GetEmpty(),
				FText::AsNumber(FMath::RoundToInt(Bonus))));
		}
		else
		{
			Text_Defense->SetText(FText::AsNumber(FMath::RoundToInt(Defense)));
		}
	}
}

void UERNPlayerDetailStatusWidget::UpdateMoveSpeedText(float MoveSpeed)
{
	if (Text_MoveSpeed) Text_MoveSpeed->SetText(FText::AsNumber(FMath::RoundToInt(MoveSpeed)));
}

void UERNPlayerDetailStatusWidget::UpdateStaggerResistText(float Resist)
{
	if (Text_StaggerResistance) Text_StaggerResistance->SetText(FText::AsNumber(FMath::RoundToInt(Resist)));
}

void UERNPlayerDetailStatusWidget::UpdateDownResistText(float Resist)
{
	if (Text_DownResistance) Text_DownResistance->SetText(FText::AsNumber(FMath::RoundToInt(Resist)));
}

void UERNPlayerDetailStatusWidget::UpdateShieldText(float Shield)
{
	if (Text_Shield) Text_Shield->SetText(FText::AsNumber(FMath::RoundToInt(Shield)));
}

void UERNPlayerDetailStatusWidget::UpdateHealthText(float Current, float Max)
{
	if (Text_Health)
	{
		float BaseMax = Max;
		float BonusMax = 0.0f;
		if (CachedASC.IsValid())
		{
			BaseMax = CachedASC->GetNumericAttributeBase(UERNAttributeSet::GetMaxHealthAttribute());
			BonusMax = Max - BaseMax;
		}

		if (BonusMax != 0.0f)
		{
			Text_Health->SetText(FText::Format(NSLOCTEXT("Status", "HealthFormatBonus", "{0} / {1} ({2}{3})"), 
				FText::AsNumber(FMath::RoundToInt(Current)), FText::AsNumber(FMath::RoundToInt(Max)),
				BonusMax > 0 ? FText::FromString(TEXT("+")) : FText::GetEmpty(),
				FText::AsNumber(FMath::RoundToInt(BonusMax))));
		}
		else
		{
			Text_Health->SetText(FText::Format(NSLOCTEXT("Status", "HealthFormat", "{0} / {1}"), 
				FText::AsNumber(FMath::RoundToInt(Current)), FText::AsNumber(FMath::RoundToInt(Max))));
		}
	}
	if (PB_Health)
	{
		PB_Health->SetPercent(Max > 0.0f ? (Current / Max) : 0.0f);
	}
}

void UERNPlayerDetailStatusWidget::UpdateManaText(float Current, float Max)
{
	if (Text_Mana)
	{
		Text_Mana->SetText(FText::Format(NSLOCTEXT("Status", "ManaFormat", "{0} / {1}"), 
			FText::AsNumber(FMath::RoundToInt(Current)), FText::AsNumber(FMath::RoundToInt(Max))));
	}
	if (PB_Mana)
	{
		PB_Mana->SetPercent(Max > 0.0f ? (Current / Max) : 0.0f);
	}
}

void UERNPlayerDetailStatusWidget::UpdateStaminaText(float Current, float Max)
{
	if (Text_Stamina)
	{
		float BaseMax = Max;
		float BonusMax = 0.0f;
		if (CachedASC.IsValid())
		{
			BaseMax = CachedASC->GetNumericAttributeBase(UERNAttributeSet::GetMaxStaminaAttribute());
			BonusMax = Max - BaseMax;
		}

		if (BonusMax != 0.0f)
		{
			Text_Stamina->SetText(FText::Format(NSLOCTEXT("Status", "StaminaFormatBonus", "{0} / {1} ({2}{3})"), 
				FText::AsNumber(FMath::RoundToInt(Current)), FText::AsNumber(FMath::RoundToInt(Max)),
				BonusMax > 0 ? FText::FromString(TEXT("+")) : FText::GetEmpty(),
				FText::AsNumber(FMath::RoundToInt(BonusMax))));
		}
		else
		{
			Text_Stamina->SetText(FText::Format(NSLOCTEXT("Status", "StaminaFormat", "{0} / {1}"), 
				FText::AsNumber(FMath::RoundToInt(Current)), FText::AsNumber(FMath::RoundToInt(Max))));
		}
	}
	if (PB_Stamina)
	{
		PB_Stamina->SetPercent(Max > 0.0f ? (Current / Max) : 0.0f);
	}
}

void UERNPlayerDetailStatusWidget::UpdateFlaskText(float Current, float Max)
{
	if (Text_Flask)
	{
		Text_Flask->SetText(FText::Format(NSLOCTEXT("Status", "FlaskFormat", "{0} / {1}"), 
			FText::AsNumber(FMath::RoundToInt(Current)), FText::AsNumber(FMath::RoundToInt(Max))));
	}
}

float UERNPlayerDetailStatusWidget::GetWeaponDamage() const
{
	if (APlayerController* PC = GetOwningPlayer())
	{
		if (AProjectERNCharacter* Character = Cast<AProjectERNCharacter>(PC->GetPawn()))
		{
			if (UERNEquipmentComponent* EquipComp = Character->GetEquipmentComponent())
			{
				if (AERNWeaponBase* Weapon = EquipComp->CurrentWeapon)
				{
					return Weapon->LightAttackDamage;
				}
			}
		}
	}
	return 0.0f;
}

void UERNPlayerDetailStatusWidget::OnEquipmentSlotChanged(const FInventoryItemEntry& Entry)
{
	if (CachedASC.IsValid())
	{
		bool bFound = false;
		float Base = CachedASC->GetGameplayAttributeValue(UERNAttributeSet::GetAttackPowerAttribute(), bFound);
		float Bonus = CachedASC->GetGameplayAttributeValue(UERNAttributeSet::GetAttackPowerBonusAttribute(), bFound);
		UpdateAttackPowerText(Base, Bonus);
	}
}
