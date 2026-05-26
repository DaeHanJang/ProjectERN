// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "ERNAttributeSet.generated.h"

#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

UCLASS()
class PROJECTERN_API UERNAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UERNAttributeSet();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// Attribute의 Value가 변경되기 직전 호출되는 함수
	// 스태미나/마나 Max를 넘지 않게 Clamp를 걸기 위함
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	
	virtual void PostGameplayEffectExecute(const struct FGameplayEffectModCallbackData& Data) override;
	
	// Health
	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_Health)
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS(UERNAttributeSet, Health)

	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_MaxHealth)
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS(UERNAttributeSet, MaxHealth)

	// Mana
	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_Mana)
	FGameplayAttributeData Mana;
	ATTRIBUTE_ACCESSORS(UERNAttributeSet, Mana)

	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_MaxMana)
	FGameplayAttributeData MaxMana;
	ATTRIBUTE_ACCESSORS(UERNAttributeSet, MaxMana)

	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_ManaRegenRate)
	FGameplayAttributeData ManaRegenRate;
	ATTRIBUTE_ACCESSORS(UERNAttributeSet, ManaRegenRate)
	
	// Stamina
	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_Stamina)
	FGameplayAttributeData Stamina;
	ATTRIBUTE_ACCESSORS(UERNAttributeSet, Stamina)

	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_MaxStamina)
	FGameplayAttributeData MaxStamina;
	ATTRIBUTE_ACCESSORS(UERNAttributeSet, MaxStamina)
	
	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_StaminaRegenRate)
	FGameplayAttributeData StaminaRegenRate;
	ATTRIBUTE_ACCESSORS(UERNAttributeSet, StaminaRegenRate)

	// Attack Power
	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_AttackPower)
	FGameplayAttributeData AttackPower;
	ATTRIBUTE_ACCESSORS(UERNAttributeSet, AttackPower)
	
	// 공격력 수치 증가
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_AttackPowerBonus, Category = "Combat")
	FGameplayAttributeData AttackPowerBonus;
	ATTRIBUTE_ACCESSORS(UERNAttributeSet, AttackPowerBonus)

	// Defense
	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_Defense)
	FGameplayAttributeData Defense;
	ATTRIBUTE_ACCESSORS(UERNAttributeSet, Defense)

	// Move Speed
	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_MoveSpeed)
	FGameplayAttributeData MoveSpeed;
	ATTRIBUTE_ACCESSORS(UERNAttributeSet, MoveSpeed)

	// Level
	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_Level)
	FGameplayAttributeData Level;
	ATTRIBUTE_ACCESSORS(UERNAttributeSet, Level)

	// Gold
	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_Gold)
	FGameplayAttributeData Gold;
	ATTRIBUTE_ACCESSORS(UERNAttributeSet, Gold)

	// Stagger Resistance — 이 값보다 낮은 StaggerPower 공격은 경직 무시
	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_StaggerResistance)
	FGameplayAttributeData StaggerResistance;
	ATTRIBUTE_ACCESSORS(UERNAttributeSet, StaggerResistance)

	// Down Resistance — 이 값 이상의 StaggerPower 공격은 다운 몽타주 재생 (StaggerResistance < DownResistance)
	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_DownResistance)
	FGameplayAttributeData DownResistance;
	ATTRIBUTE_ACCESSORS(UERNAttributeSet, DownResistance)
	
	// Flask
	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_MaxFlaskQuantity)
	FGameplayAttributeData MaxFlaskQuantity;
	ATTRIBUTE_ACCESSORS(UERNAttributeSet, MaxFlaskQuantity)
	
	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_FlaskQuantity)
	FGameplayAttributeData FlaskQuantity;
	ATTRIBUTE_ACCESSORS(UERNAttributeSet, FlaskQuantity)
	
	// Shield
	UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_Shield)
	FGameplayAttributeData Shield;
	ATTRIBUTE_ACCESSORS(UERNAttributeSet, Shield)

protected:
	UFUNCTION()
	virtual void OnRep_Health(const FGameplayAttributeData& OldHealth);

	UFUNCTION()
	virtual void OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth);

	UFUNCTION()
	virtual void OnRep_Mana(const FGameplayAttributeData& OldMana);

	UFUNCTION()
	virtual void OnRep_MaxMana(const FGameplayAttributeData& OldMaxMana);

	UFUNCTION()
	virtual void OnRep_ManaRegenRate(const FGameplayAttributeData& OldManaRegenRate);
	
	UFUNCTION()
	virtual void OnRep_Stamina(const FGameplayAttributeData& OldStamina);

	UFUNCTION()
	virtual void OnRep_MaxStamina(const FGameplayAttributeData& OldMaxStamina);

	UFUNCTION()
	virtual void OnRep_StaminaRegenRate(const FGameplayAttributeData& OldStaminaRegenRate);
	
	UFUNCTION()
	virtual void OnRep_AttackPower(const FGameplayAttributeData& OldAttackPower);
	
	UFUNCTION()
	void OnRep_AttackPowerBonus(const FGameplayAttributeData& OldAttackPowerBonus);

	UFUNCTION()
	virtual void OnRep_Defense(const FGameplayAttributeData& OldDefense);

	UFUNCTION()
	virtual void OnRep_MoveSpeed(const FGameplayAttributeData& OldMoveSpeed);

	UFUNCTION()
	virtual void OnRep_Level(const FGameplayAttributeData& OldLevel);

	UFUNCTION()
	virtual void OnRep_Gold(const FGameplayAttributeData& OldGold);

	UFUNCTION()
	virtual void OnRep_StaggerResistance(const FGameplayAttributeData& OldStaggerResistance);

	UFUNCTION()
	virtual void OnRep_DownResistance(const FGameplayAttributeData& OldDownResistance);
	
	UFUNCTION()
	virtual void OnRep_MaxFlaskQuantity(const FGameplayAttributeData& OldMaxFlaskQuantity);
	
	UFUNCTION()
	virtual void OnRep_FlaskQuantity(const FGameplayAttributeData& OldFlaskQuantity);
	
	UFUNCTION()
	virtual void OnRep_Shield(const FGameplayAttributeData& OldShield);
};
