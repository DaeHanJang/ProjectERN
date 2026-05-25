// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/World/ERNMinimapWidget.h"

#include "MinimapMarkerWidget.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "World/ERNMinimapSubsystem.h"
#include "World/ERNMinimapTargetPoint.h"
#include "World/Data/ERNMinimapIconDataAsset.h"

void UERNMinimapWidget::PlayOpenAnimation()
{
	if (FadeIn)
	{
		PlayAnimation(FadeIn);
	}
	else
	{
		UE_LOG(LogTemp,Warning,TEXT("MinimapWidget FadeIn is null"));
	}
}

void UERNMinimapWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	SetVisibility(ESlateVisibility::Collapsed);
	
	
	if (UERNMinimapSubsystem* Subsystem = GetWorld() ? GetWorld()->GetSubsystem<UERNMinimapSubsystem>() : nullptr)
	{
		TargetsChangedHandle = Subsystem->OnTargetsChanged.AddUObject(
			this,
			&UERNMinimapWidget::HandleTargetsChanged);
	}

	bStaticMarkersDirty = true;
}

void UERNMinimapWidget::NativeDestruct()
{
	if (TargetsChangedHandle.IsValid())
	{
		if (UERNMinimapSubsystem* Subsystem = GetWorld() ? GetWorld()->GetSubsystem<UERNMinimapSubsystem>() : nullptr)
		{
			Subsystem->OnTargetsChanged.Remove(TargetsChangedHandle);
			TargetsChangedHandle.Reset();
		}
	}
	
	Super::NativeDestruct();
}

FReply UERNMinimapWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		SetKeyboardFocus();
		return FReply::Handled();
	}
	
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UERNMinimapWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();

	if (Key == EKeys::W ||
		Key == EKeys::A ||
		Key == EKeys::S ||
		Key == EKeys::D)
	{
		return FReply::Unhandled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

FVector2D UERNMinimapWidget::WorldToMapPosition(const FVector& WorldLocation) const
{
	const FVector2D WorldSize = WorldMax - WorldMin;
	
	if (FMath::IsNearlyZero(WorldSize.X) || FMath::IsNearlyZero(WorldSize.Y))
	{
		return FVector2D::ZeroVector;
	}
	
	const float NormalizedX = FMath::Clamp((WorldLocation.X - WorldMin.X) / WorldSize.X, 0.f, 1.f);
	const float NormalizedY = FMath::Clamp((WorldLocation.Y - WorldMin.Y) / WorldSize.Y, 0.f, 1.f);
	
	return FVector2D(NormalizedX * MapSize.X, (1 - NormalizedY) * MapSize.Y);
}

void UERNMinimapWidget::RebuildStaticMarkers()
{
	if (MarkerLayer == nullptr || MarkerWidgetClass ==  nullptr || IconDataAsset == nullptr)
	{
		return;
	}
	
	MarkerLayer->ClearChildren();
	
	UERNMinimapSubsystem* Subsystem = GetWorld() ? GetWorld()->GetSubsystem<UERNMinimapSubsystem>() : nullptr;
	
	if (Subsystem == nullptr)
	{
		return;
	}
	TArray<AERNMinimapTargetPoint*> Targets;
	Subsystem->GetTargets(Targets);
	
	for (AERNMinimapTargetPoint* Target : Targets)
	{
		if (IsValid(Target) == false || Target->bVisibleOnMinimap == false)
		{
			continue;
		}
		
		UTexture2D* IconTexture = IconDataAsset->FindIconTexture(Target->IconType);
		if (IconTexture == nullptr)
		{
			continue;
		}
		
		UMinimapMarkerWidget* Marker = CreateWidget<UMinimapMarkerWidget>(this, MarkerWidgetClass);
		if (Marker == nullptr)
		{
			continue;
		}
		
		Marker->SetMarkerData(IconTexture, Target);
		
		if (UCanvasPanelSlot* MarkerSlot = MarkerLayer->AddChildToCanvas(Marker))
		{
			MarkerSlot->SetAutoSize(true);
			MarkerSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			MarkerSlot->SetPosition(WorldToMapPosition(Target->GetMinimapWorldLocation()));
		}
	}
	
	bStaticMarkersDirty = false;
}

void UERNMinimapWidget::RefreshStaticMarkers()
{
	if (bStaticMarkersDirty)
	{
		RebuildStaticMarkers();
	}
}

void UERNMinimapWidget::HandleTargetsChanged()
{
	bStaticMarkersDirty = true;
	
	// 보이는 대상만 최신화
	if (GetVisibility() == ESlateVisibility::Visible)
	{
		RebuildStaticMarkers();
	}
}
