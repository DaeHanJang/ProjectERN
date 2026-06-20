// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/ERNAccountStatWidget.h"

#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/CheckBox.h"
#include "Core/ERNGameInstance.h"
#include "Character/Player/ProjectERNCharacter.h"
#include "GameFramework/PlayerController.h"

// 포인트당 증가량 (ApplyAccountBuffsInternal과 동일하게 표시)
namespace
{
	constexpr int32 PerHealth = 40;
	constexpr int32 PerMana = 20;
	constexpr int32 PerStamina = 20;
	constexpr int32 PerDefense = 4;
	constexpr int32 PerAttack = 8;
	constexpr float PerLifestealPercent = 0.2f; // %/pt
	constexpr int32 PerGold = 40;
}

void UERNAccountStatWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Btn_HealthPlus)     Btn_HealthPlus->OnClicked.AddDynamic(this, &UERNAccountStatWidget::OnPlusHealth);
	if (Btn_HealthMinus)    Btn_HealthMinus->OnClicked.AddDynamic(this, &UERNAccountStatWidget::OnMinusHealth);
	if (Btn_ManaPlus)       Btn_ManaPlus->OnClicked.AddDynamic(this, &UERNAccountStatWidget::OnPlusMana);
	if (Btn_ManaMinus)      Btn_ManaMinus->OnClicked.AddDynamic(this, &UERNAccountStatWidget::OnMinusMana);
	if (Btn_StaminaPlus)    Btn_StaminaPlus->OnClicked.AddDynamic(this, &UERNAccountStatWidget::OnPlusStamina);
	if (Btn_StaminaMinus)   Btn_StaminaMinus->OnClicked.AddDynamic(this, &UERNAccountStatWidget::OnMinusStamina);
	if (Btn_DefensePlus)    Btn_DefensePlus->OnClicked.AddDynamic(this, &UERNAccountStatWidget::OnPlusDefense);
	if (Btn_DefenseMinus)   Btn_DefenseMinus->OnClicked.AddDynamic(this, &UERNAccountStatWidget::OnMinusDefense);
	if (Btn_AttackPlus)     Btn_AttackPlus->OnClicked.AddDynamic(this, &UERNAccountStatWidget::OnPlusAttack);
	if (Btn_AttackMinus)    Btn_AttackMinus->OnClicked.AddDynamic(this, &UERNAccountStatWidget::OnMinusAttack);
	if (Btn_LifestealPlus)  Btn_LifestealPlus->OnClicked.AddDynamic(this, &UERNAccountStatWidget::OnPlusLifesteal);
	if (Btn_LifestealMinus) Btn_LifestealMinus->OnClicked.AddDynamic(this, &UERNAccountStatWidget::OnMinusLifesteal);
	if (Btn_GoldPlus)       Btn_GoldPlus->OnClicked.AddDynamic(this, &UERNAccountStatWidget::OnPlusGold);
	if (Btn_GoldMinus)      Btn_GoldMinus->OnClicked.AddDynamic(this, &UERNAccountStatWidget::OnMinusGold);

	if (Btn_Reset)          Btn_Reset->OnClicked.AddDynamic(this, &UERNAccountStatWidget::OnResetClicked);
	if (Btn_Back)           Btn_Back->OnClicked.AddDynamic(this, &UERNAccountStatWidget::OnBackClicked);

	// 하드모드 체크박스: 호스트(리슨서버 = 로컬 PC HasAuthority)에게만 표시
	if (Check_HardMode)
	{
		const APlayerController* PC = GetOwningPlayer();
		const bool bIsHost = PC && PC->HasAuthority();

		Check_HardMode->SetVisibility(bIsHost ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);

		if (bIsHost)
		{
			if (const UERNGameInstance* GI = Cast<UERNGameInstance>(GetGameInstance()))
			{
				Check_HardMode->SetIsChecked(GI->IsHardModeEnabled());
			}
			Check_HardMode->OnCheckStateChanged.AddDynamic(this, &UERNAccountStatWidget::OnHardModeChanged);
		}
	}

	Refresh();
}

FText UERNAccountStatWidget::FormatStatLine(EAccountStat Stat) const
{
	const UERNGameInstance* GI = Cast<UERNGameInstance>(GetGameInstance());
	const int32 C = GI ? GI->GetInvestedPoints(Stat) : 0;

	switch (Stat)
	{
	case EAccountStat::Health:    return FText::FromString(FString::Printf(TEXT("Lv %d  (+%d)"), C, PerHealth * C));
	case EAccountStat::Mana:      return FText::FromString(FString::Printf(TEXT("Lv %d  (+%d)"), C, PerMana * C));
	case EAccountStat::Stamina:   return FText::FromString(FString::Printf(TEXT("Lv %d  (+%d)"), C, PerStamina * C));
	case EAccountStat::Defense:   return FText::FromString(FString::Printf(TEXT("Lv %d  (+%d)"), C, PerDefense * C));
	case EAccountStat::Attack:    return FText::FromString(FString::Printf(TEXT("Lv %d  (+%d)"), C, PerAttack * C));
	case EAccountStat::Lifesteal: return FText::FromString(FString::Printf(TEXT("Lv %d  (+%.1f%%)"), C, PerLifestealPercent * C));
	case EAccountStat::Gold:      return FText::FromString(FString::Printf(TEXT("Lv %d  (+%d)"), C, PerGold * C));
	default:                      return FText::FromString(TEXT("Lv 0"));
	}
}

void UERNAccountStatWidget::Refresh()
{
	const UERNGameInstance* GI = Cast<UERNGameInstance>(GetGameInstance());
	if (!GI)
	{
		return;
	}

	if (Text_AccountLevel)
	{
		Text_AccountLevel->SetText(FText::FromString(FString::Printf(TEXT("계정 레벨: %d"), GI->GetAccountLevel())));
	}
	if (Text_AvailablePoints)
	{
		Text_AvailablePoints->SetText(FText::FromString(FString::Printf(TEXT("보유 포인트: %d"), GI->GetAvailablePoints())));
	}

	if (Text_Health)    Text_Health->SetText(FormatStatLine(EAccountStat::Health));
	if (Text_Mana)      Text_Mana->SetText(FormatStatLine(EAccountStat::Mana));
	if (Text_Stamina)   Text_Stamina->SetText(FormatStatLine(EAccountStat::Stamina));
	if (Text_Defense)   Text_Defense->SetText(FormatStatLine(EAccountStat::Defense));
	if (Text_Attack)    Text_Attack->SetText(FormatStatLine(EAccountStat::Attack));
	if (Text_Lifesteal) Text_Lifesteal->SetText(FormatStatLine(EAccountStat::Lifesteal));
	if (Text_Gold)      Text_Gold->SetText(FormatStatLine(EAccountStat::Gold));
}

void UERNAccountStatWidget::NotifyPawnRefresh()
{
	if (AProjectERNCharacter* Char = Cast<AProjectERNCharacter>(GetOwningPlayerPawn()))
	{
		Char->RefreshAccountBuffs();
	}
}

void UERNAccountStatWidget::Invest(EAccountStat Stat)
{
	if (UERNGameInstance* GI = Cast<UERNGameInstance>(GetGameInstance()))
	{
		if (GI->InvestAccountPoint(Stat))
		{
			NotifyPawnRefresh();
		}
	}
	Refresh();
}

void UERNAccountStatWidget::Uninvest(EAccountStat Stat)
{
	if (UERNGameInstance* GI = Cast<UERNGameInstance>(GetGameInstance()))
	{
		if (GI->UninvestAccountPoint(Stat))
		{
			NotifyPawnRefresh();
		}
	}
	Refresh();
}

void UERNAccountStatWidget::OnPlusHealth() { Invest(EAccountStat::Health); }
void UERNAccountStatWidget::OnMinusHealth() { Uninvest(EAccountStat::Health); }
void UERNAccountStatWidget::OnPlusMana() { Invest(EAccountStat::Mana); }
void UERNAccountStatWidget::OnMinusMana() { Uninvest(EAccountStat::Mana); }
void UERNAccountStatWidget::OnPlusStamina() { Invest(EAccountStat::Stamina); }
void UERNAccountStatWidget::OnMinusStamina() { Uninvest(EAccountStat::Stamina); }
void UERNAccountStatWidget::OnPlusDefense() { Invest(EAccountStat::Defense); }
void UERNAccountStatWidget::OnMinusDefense() { Uninvest(EAccountStat::Defense); }
void UERNAccountStatWidget::OnPlusAttack() { Invest(EAccountStat::Attack); }
void UERNAccountStatWidget::OnMinusAttack() { Uninvest(EAccountStat::Attack); }
void UERNAccountStatWidget::OnPlusLifesteal() { Invest(EAccountStat::Lifesteal); }
void UERNAccountStatWidget::OnMinusLifesteal() { Uninvest(EAccountStat::Lifesteal); }
void UERNAccountStatWidget::OnPlusGold() { Invest(EAccountStat::Gold); }
void UERNAccountStatWidget::OnMinusGold() { Uninvest(EAccountStat::Gold); }

void UERNAccountStatWidget::OnResetClicked()
{
	if (UERNGameInstance* GI = Cast<UERNGameInstance>(GetGameInstance()))
	{
		GI->ResetAccountPoints();
	}
	NotifyPawnRefresh();
	Refresh();
}

void UERNAccountStatWidget::OnBackClicked()
{
	// 기본 구현이 OnWidgetClosed 브로드캐스트 → 액터(HandleWidgetClosed)가 제거 + 입력 복구
	BP_PlayCloseAnimation();
}

void UERNAccountStatWidget::OnHardModeChanged(bool bIsChecked)
{
	// 호스트 GI(=서버 GI)에 저장. 서버 데미지/골드 로직 + 클리어 보상이 이 값을 참조
	if (UERNGameInstance* GI = Cast<UERNGameInstance>(GetGameInstance()))
	{
		GI->SetHardModeEnabled(bIsChecked);
	}
}
