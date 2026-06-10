// Copyright Epic Games, Inc. All Rights Reserved.

#include "MyStatusWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerController.h"
#include "Character/Player/ERNPlayerState.h"
#include "UI/ERNBuffListWidget.h"
#include "AbilitySystemComponent.h"

void UMyStatusWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// PlayerState 캐시
	if (APlayerController* PC = GetOwningPlayer())
	{
		CachedPlayerState = PC->GetPlayerState<AERNPlayerState>();
		if (CachedPlayerState)
		{
			UE_LOG(LogTemp, Warning, TEXT("[BuffUI] MyStatusWidget::NativeConstruct - CachedPlayerState successfully grabbed. PS Name: %s"), *CachedPlayerState->GetName());
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[BuffUI] MyStatusWidget::NativeConstruct - PC exists, but PlayerState is NULL!"));
		}
	}
}

void UMyStatusWidget::SetTargetPlayerState(AERNPlayerState* NewPlayerState)
{
	if (CachedPlayerState != NewPlayerState)
	{
		UE_LOG(LogTemp, Warning, TEXT("[BuffUI] MyStatusWidget::SetTargetPlayerState called! Old: %s, New: %s"), 
			CachedPlayerState ? *CachedPlayerState->GetName() : TEXT("NULL"),
			NewPlayerState ? *NewPlayerState->GetName() : TEXT("NULL"));
			
		CachedPlayerState = NewPlayerState;
		bIsBuffListInitialized = false; // 새 타겟으로 버프 리스트 초기화 필요
	}
}

void UMyStatusWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (CachedPlayerState)
	{
		if (!bIsBuffListInitialized)
		{
			if (BuffList)
			{
				BuffList->InitializeWithPlayerState(CachedPlayerState);
				bIsBuffListInitialized = true;
				UE_LOG(LogTemp, Warning, TEXT("[BuffUI] MyStatusWidget::NativeTick - successfully initialized BuffList with PS: %s"), *CachedPlayerState->GetName());
			}
			else
			{
				static float LogTimer = 0.f;
				LogTimer -= InDeltaTime;
				if (LogTimer <= 0.f)
				{
					UE_LOG(LogTemp, Error, TEXT("[BuffUI] MyStatusWidget::NativeTick - BuffList is NULL!"));
					LogTimer = 1.0f;
				}
			}
		}
		
		static float TickLogTimer = 0.f;
		TickLogTimer -= InDeltaTime;
		if (TickLogTimer <= 0.f)
		{
			// 너무 많이 뜨면 안 되니 5초에 한 번만 로그 확인
			// UE_LOG(LogTemp, Log, TEXT("[BuffUI] MyStatusWidget is ticking with PS: %s"), *CachedPlayerState->GetName());
			TickLogTimer = 5.0f;
		}

		UpdateHealth();
		UpdateMana();
		UpdateStamina();
	}
}

void UMyStatusWidget::UpdateHealth()
{
	float Current = CachedPlayerState->GetCurrentHealth();
	float Max = CachedPlayerState->GetMaxHealth();

	if (HealthBar && Max > 0.f)
	{
		HealthBar->SetPercent(Current / Max);
	}

	if (HealthText)
	{
		FText HealthString = FText::Format(
			FText::FromString("{0} / {1}"),
			FText::AsNumber(FMath::RoundToInt(Current)),
			FText::AsNumber(FMath::RoundToInt(Max))
		);
		HealthText->SetText(HealthString);
	}
}

void UMyStatusWidget::UpdateMana()
{
	float Current = CachedPlayerState->GetCurrentMana();
	float Max = CachedPlayerState->GetMaxMana();

	if (ManaBar && Max > 0.f)
	{
		ManaBar->SetPercent(Current / Max);
	}

	if (ManaText)
	{
		FText ManaString = FText::Format(
			FText::FromString("{0} / {1}"),
			FText::AsNumber(FMath::RoundToInt(Current)),
			FText::AsNumber(FMath::RoundToInt(Max))
		);
		ManaText->SetText(ManaString);
	}
}

void UMyStatusWidget::UpdateStamina()
{
	float Current = CachedPlayerState->GetCurrentStamina();
	float Max = CachedPlayerState->GetMaxStamina();

	if (StaminaBar && Max > 0.f)
	{
		StaminaBar->SetPercent(Current / Max);
	}

	if (StaminaText)
	{
		FText StaminaString = FText::Format(
			FText::FromString("{0} / {1}"),
			FText::AsNumber(FMath::RoundToInt(Current)),
			FText::AsNumber(FMath::RoundToInt(Max))
		);
		StaminaText->SetText(StaminaString);
	}
}
