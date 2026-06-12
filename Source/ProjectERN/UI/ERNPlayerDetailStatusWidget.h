// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayEffectTypes.h"
#include "Inventory/Data/ERNInventoryList.h"
#include "ERNPlayerDetailStatusWidget.generated.h"

class UAbilitySystemComponent;
class UERNAttributeSet;

/**
 * 일시정지 창 등에서 플레이어의 전체 능력치를 보여주는 상태창 위젯 베이스 클래스
 */
UCLASS()
class PROJECTERN_API UERNPlayerDetailStatusWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 위젯이 화면에 띄워질 때 자동으로 능력치 초기화 및 델리게이트 연동 수행
	virtual void NativeConstruct() override;
	
	// 화면에서 내려갈 때 델리게이트 해제
	virtual void NativeDestruct() override;

	// ASC 초기화 시도 (성공 시 true 반환)
	UFUNCTION(BlueprintCallable, Category = "Player Status")
	bool TryInitASC();

	// 수동으로 모든 능력치 UI 갱신을 요청할 때 블루프린트에서 호출할 수 있는 함수
	UFUNCTION(BlueprintCallable, Category = "Player Status")
	void RefreshAllAttributes();

protected:
	// 위젯 바인딩 (이름을 동일하게 맞추면 자동 연결됩니다)
	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* Text_Level;

	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* Text_Gold;

	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* Text_AttackPower;

	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* Text_Defense;

	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* Text_MoveSpeed;

	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* Text_StaggerResistance;

	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* Text_DownResistance;

	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* Text_Shield;

	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* Text_Health;

	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* Text_Mana;

	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* Text_Stamina;

	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* Text_Flask;

	UPROPERTY(meta = (BindWidgetOptional))
	class UProgressBar* PB_Health;

	UPROPERTY(meta = (BindWidgetOptional))
	class UProgressBar* PB_Mana;

	UPROPERTY(meta = (BindWidgetOptional))
	class UProgressBar* PB_Stamina;

	// 내부 텍스트 갱신 헬퍼
	void UpdateLevelText(float Level);
	void UpdateGoldText(float Gold);
	void UpdateAttackPowerText(float Base, float Bonus);
	void UpdateDefenseText(float Defense);
	void UpdateMoveSpeedText(float MoveSpeed);
	void UpdateStaggerResistText(float Resist);
	void UpdateDownResistText(float Resist);
	void UpdateShieldText(float Shield);
	void UpdateHealthText(float Current, float Max);
	void UpdateManaText(float Current, float Max);
	void UpdateStaminaText(float Current, float Max);
	void UpdateFlaskText(float Current, float Max);

private:
	// 캐싱된 ASC
	TWeakObjectPtr<UAbilitySystemComponent> CachedASC;

	UFUNCTION()
	void OnEquipmentSlotChanged(const FInventoryItemEntry& Entry);

	float GetWeaponDamage() const;

	// 델리게이트 핸들 보관용
	FDelegateHandle HealthChangedDelegateHandle;
	FDelegateHandle MaxHealthChangedDelegateHandle;
	FDelegateHandle ManaChangedDelegateHandle;
	FDelegateHandle MaxManaChangedDelegateHandle;
	FDelegateHandle StaminaChangedDelegateHandle;
	FDelegateHandle MaxStaminaChangedDelegateHandle;
	FDelegateHandle AttackPowerChangedDelegateHandle;
	FDelegateHandle AttackPowerBonusChangedDelegateHandle;
	FDelegateHandle DefenseChangedDelegateHandle;
	FDelegateHandle MoveSpeedChangedDelegateHandle;
	FDelegateHandle LevelChangedDelegateHandle;
	FDelegateHandle GoldChangedDelegateHandle;
	FDelegateHandle StaggerResistChangedDelegateHandle;
	FDelegateHandle DownResistChangedDelegateHandle;
	FDelegateHandle FlaskChangedDelegateHandle;
	FDelegateHandle MaxFlaskChangedDelegateHandle;
	FDelegateHandle ShieldChangedDelegateHandle;

	// 개별 어트리뷰트 콜백 함수들
	void HealthChanged(const FOnAttributeChangeData& Data);
	void MaxHealthChanged(const FOnAttributeChangeData& Data);
	void ManaChanged(const FOnAttributeChangeData& Data);
	void MaxManaChanged(const FOnAttributeChangeData& Data);
	void StaminaChanged(const FOnAttributeChangeData& Data);
	void MaxStaminaChanged(const FOnAttributeChangeData& Data);
	void AttackPowerChanged(const FOnAttributeChangeData& Data);
	void AttackPowerBonusChanged(const FOnAttributeChangeData& Data);
	void DefenseChanged(const FOnAttributeChangeData& Data);
	void MoveSpeedChanged(const FOnAttributeChangeData& Data);
	void LevelChanged(const FOnAttributeChangeData& Data);
	void GoldChanged(const FOnAttributeChangeData& Data);
	void StaggerResistChanged(const FOnAttributeChangeData& Data);
	void DownResistChanged(const FOnAttributeChangeData& Data);
	void FlaskChanged(const FOnAttributeChangeData& Data);
	void MaxFlaskChanged(const FOnAttributeChangeData& Data);
	void ShieldChanged(const FOnAttributeChangeData& Data);
};
