// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/ERNInteractableWidget.h"
#include "Core/ERNSaveSettings.h" // EAccountStat
#include "ERNAccountStatWidget.generated.h"

class UTextBlock;
class UButton;

/**
 * 계정 영구 스탯 투자 위젯 (로직 + 바인딩만 C++, 외형/레이아웃은 WBP에서 제작).
 * WBP에서 아래 BindWidget 이름과 동일하게 위젯을 배치해야 합니다.
 * 투자/회수/초기화는 GameInstance(로컬 세이브)로 처리하고 pawn->RefreshAccountBuffs()로 즉시 반영.
 * 닫기는 UERNInteractableWidget 기본 흐름(OnWidgetClosed) 사용.
 */
UCLASS()
class PROJECTERN_API UERNAccountStatWidget : public UERNInteractableWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

	// ===== BindWidget (WBP에서 동일 이름으로 배치) =====
	UPROPERTY(meta = (BindWidget)) UTextBlock* Text_AccountLevel;
	UPROPERTY(meta = (BindWidget)) UTextBlock* Text_AvailablePoints;

	UPROPERTY(meta = (BindWidget)) UTextBlock* Text_Health;
	UPROPERTY(meta = (BindWidget)) UTextBlock* Text_Mana;
	UPROPERTY(meta = (BindWidget)) UTextBlock* Text_Stamina;
	UPROPERTY(meta = (BindWidget)) UTextBlock* Text_Defense;
	UPROPERTY(meta = (BindWidget)) UTextBlock* Text_Attack;
	UPROPERTY(meta = (BindWidget)) UTextBlock* Text_Lifesteal;
	UPROPERTY(meta = (BindWidget)) UTextBlock* Text_Gold;

	UPROPERTY(meta = (BindWidget)) UButton* Btn_HealthPlus;
	UPROPERTY(meta = (BindWidget)) UButton* Btn_HealthMinus;
	UPROPERTY(meta = (BindWidget)) UButton* Btn_ManaPlus;
	UPROPERTY(meta = (BindWidget)) UButton* Btn_ManaMinus;
	UPROPERTY(meta = (BindWidget)) UButton* Btn_StaminaPlus;
	UPROPERTY(meta = (BindWidget)) UButton* Btn_StaminaMinus;
	UPROPERTY(meta = (BindWidget)) UButton* Btn_DefensePlus;
	UPROPERTY(meta = (BindWidget)) UButton* Btn_DefenseMinus;
	UPROPERTY(meta = (BindWidget)) UButton* Btn_AttackPlus;
	UPROPERTY(meta = (BindWidget)) UButton* Btn_AttackMinus;
	UPROPERTY(meta = (BindWidget)) UButton* Btn_LifestealPlus;
	UPROPERTY(meta = (BindWidget)) UButton* Btn_LifestealMinus;
	UPROPERTY(meta = (BindWidget)) UButton* Btn_GoldPlus;
	UPROPERTY(meta = (BindWidget)) UButton* Btn_GoldMinus;

	UPROPERTY(meta = (BindWidget)) UButton* Btn_Reset;
	UPROPERTY(meta = (BindWidget)) UButton* Btn_Back;

private:
	// 표시값 갱신
	void Refresh();

	// 투자/회수/알림
	void Invest(EAccountStat Stat);
	void Uninvest(EAccountStat Stat);
	void NotifyPawnRefresh();

	// 스탯 투자량/버프 표시 문자열
	FText FormatStatLine(EAccountStat Stat) const;

	// 버튼 핸들러 (UButton::OnClicked는 무인자라 스탯별 개별 함수 필요)
	UFUNCTION() void OnPlusHealth();
	UFUNCTION() void OnMinusHealth();
	UFUNCTION() void OnPlusMana();
	UFUNCTION() void OnMinusMana();
	UFUNCTION() void OnPlusStamina();
	UFUNCTION() void OnMinusStamina();
	UFUNCTION() void OnPlusDefense();
	UFUNCTION() void OnMinusDefense();
	UFUNCTION() void OnPlusAttack();
	UFUNCTION() void OnMinusAttack();
	UFUNCTION() void OnPlusLifesteal();
	UFUNCTION() void OnMinusLifesteal();
	UFUNCTION() void OnPlusGold();
	UFUNCTION() void OnMinusGold();
	UFUNCTION() void OnResetClicked();
	UFUNCTION() void OnBackClicked();
};
