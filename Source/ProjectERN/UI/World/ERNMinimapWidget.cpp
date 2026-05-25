// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/World/ERNMinimapWidget.h"

#include "MinimapMarkerWidget.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "World/ERNMinimapSubsystem.h"
#include "World/ERNMinimapTargetPoint.h"
#include "World/Data/ERNMinimapIconDataAsset.h"
#include "Character/Player/ERNPlayerController.h"

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
	// 좌클릭 : 미니맵 잡기
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		SetKeyboardFocus();
		return FReply::Handled();
	}
	
	// 드래그 : 미니맵 이동

	// 휠 클릭 : 핀
	if (InMouseEvent.GetEffectingButton() == EKeys::MiddleMouseButton)
	{
		if (MarkerLayer == nullptr)
		{
			return FReply::Handled();
		}
		
		const FVector2D MapPosition = MarkerLayer->GetCachedGeometry().AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
		
		const FVector WorldLocation = MapToWorldPosition(MapPosition);
		
		if (AERNPlayerController* PC = Cast<AERNPlayerController>(GetOwningPlayer()))
		{
			PC->Server_RequestCreateMinimapPin(WorldLocation);
		}
		
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

FVector UERNMinimapWidget::MapToWorldPosition(const FVector2D& MapPosition) const
{
	const FVector2D WorldSize = WorldMax - WorldMin;
	
	if (FMath::IsNearlyZero(MapSize.X) || FMath::IsNearlyZero(MapSize.Y))
	{
		return FVector::ZeroVector;
	}
	
	const float NormalizedX = FMath::Clamp(MapPosition.X / MapSize.X, 0.f, 1.f);
	const float NormalizedY = FMath::Clamp(MapPosition.Y / MapSize.Y, 0.f, 1.f);
	
	const float WorldX = WorldMin.X + NormalizedX * WorldSize.X;
	const float WorldY = WorldMin.Y + (1.f - NormalizedY) * WorldSize.Y;
	
	return FVector(WorldX, WorldY, 0.f);
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
