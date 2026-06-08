// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/ERNDownedStatusWidget.h"

#include "Character/Player/ProjectERNCharacter.h"
#include "Components/ERNDownedComponent.h"

void UERNDownedStatusWidget::SetObservedCharacter(AProjectERNCharacter* InCharacter)
{
	if (CachedCharacter.Get() == InCharacter)
	{
		UpdateDownedVisual();
		return;
	}

	UnbindFromDownedComponent();

	CachedCharacter = InCharacter;

	BindToDownedComponent();
	UpdateDownedVisual();
}

void UERNDownedStatusWidget::HideWidgetForLocal(bool bInHide)
{
	bHideWidgetForLocal = bInHide;
	
	if (CachedCharacter.IsValid())
	{
		UpdateDownedVisual();
	}
	else
	{
		SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UERNDownedStatusWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 위젯 숨기기
	SetVisibility(ESlateVisibility::Collapsed);
}

void UERNDownedStatusWidget::NativeDestruct()
{
	// 델리게이트 해제
	UnbindFromDownedComponent();

	Super::NativeDestruct();
}


void UERNDownedStatusWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// UI 갱신
	UpdateDownedVisual();
}

void UERNDownedStatusWidget::HandleDownedGaugeChanged(float CurrentHealth, float MaxHealth, int32 PenaltyStacks)
{
	UpdateDownedVisual();
}

void UERNDownedStatusWidget::BindToDownedComponent()
{
	if (!CachedCharacter.IsValid())
	{
		return;
	}

	UERNDownedComponent* DownedComponent = CachedCharacter->GetDownedComponent();
	if (!DownedComponent)
	{
		return;
	}

	BoundDownedComponent = DownedComponent;
	DownedComponent->OnDownedGaugeChanged.AddUniqueDynamic(this, &UERNDownedStatusWidget::HandleDownedGaugeChanged);
}

void UERNDownedStatusWidget::UnbindFromDownedComponent()
{
	if (UERNDownedComponent* DownedComponent = BoundDownedComponent.Get())
	{
		DownedComponent->OnDownedGaugeChanged.RemoveDynamic(this, &UERNDownedStatusWidget::HandleDownedGaugeChanged);
	}

	BoundDownedComponent = nullptr;
}

void UERNDownedStatusWidget::UpdateDownedVisual()
{
	if (!CachedCharacter.IsValid())
	{
		if (bWasVisible)
		{
			OnDownedVisualHidden();
			bWasVisible = false;
		}

		SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	if (bHideWidgetForLocal && CachedCharacter->IsLocallyControlled())
	{
		if (bWasVisible)
		{
			OnDownedVisualHidden();
			bWasVisible = false;
		}

		SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	const bool bShouldShow = CachedCharacter->IsDowned();

	if (!bShouldShow)
	{
		if (bWasVisible)
		{
			OnDownedVisualHidden();
			bWasVisible = false;
		}

		SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	bWasVisible = true;

	const UERNDownedComponent* DownedComponent = CachedCharacter->GetDownedComponent();
	
	OnDownedVisualUpdated(
		DownedComponent ? DownedComponent->GetDownedHealthGlobalPercent() : 0.f,
		DownedComponent ? DownedComponent->GetActiveDownedMaxGlobalPercent() : 0.f,
		CachedCharacter->GetDownedRespawnRemainingPercent(),
		CachedCharacter->ShouldShowDownedRespawnCountdown(),
		DownedComponent ? DownedComponent->GetPenaltyStacks() : 0,
		DownedComponent ? DownedComponent->GetMaxPenaltyStacks() : 0);
}
