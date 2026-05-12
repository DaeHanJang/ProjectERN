// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/ERNBossHealthBarWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "TimerManager.h"

void UERNBossHealthBarWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 초기 누적 데미지 숨김
	if (AccumulatedDamageText)
	{
		AccumulatedDamageText->SetVisibility(ESlateVisibility::Hidden);
	}
}

void UERNBossHealthBarWidget::SetBossInfo(const FText& Name, float HealthPercent)
{
	if (BossNameText)
	{
		BossNameText->SetText(Name);
	}

	if (HealthBar)
	{
		HealthBar->SetPercent(HealthPercent);
	}

	// 새 보스 시작 시 누적 데미지 리셋
	AccumulatedDamage = 0.f;
	if (AccumulatedDamageText)
	{
		AccumulatedDamageText->SetVisibility(ESlateVisibility::Hidden);
	}
}

void UERNBossHealthBarWidget::UpdateHealth(float HealthPercent, float DamageDealt)
{
	if (HealthBar)
	{
		HealthBar->SetPercent(HealthPercent);
	}

	// 데미지가 있을 때만 누적 처리
	if (DamageDealt > 0.f)
	{
		AccumulatedDamage += DamageDealt;
		UpdateAccumulatedDamageText();

		// 타이머 리셋 (3초 후 리셋)
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(DamageResetTimerHandle);
			World->GetTimerManager().SetTimer(
				DamageResetTimerHandle,
				this,
				&UERNBossHealthBarWidget::ResetAccumulatedDamage,
				DamageResetTime,
				false
			);
		}
	}
}

void UERNBossHealthBarWidget::ResetAccumulatedDamage()
{
	AccumulatedDamage = 0.f;

	if (AccumulatedDamageText)
	{
		AccumulatedDamageText->SetVisibility(ESlateVisibility::Hidden);
	}
}

void UERNBossHealthBarWidget::UpdateAccumulatedDamageText()
{
	if (AccumulatedDamageText)
	{
		AccumulatedDamageText->SetVisibility(ESlateVisibility::Visible);
		AccumulatedDamageText->SetText(FText::AsNumber(FMath::RoundToInt(AccumulatedDamage)));
	}
}
