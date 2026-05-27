// Fill out your copyright notice in the Description page of Project Settings.


#include "MinimapNightRainZoneWidget.h"

#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"

void UMinimapNightRainZoneWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	
	ApplyZoneTexture();
}

void UMinimapNightRainZoneWidget::ApplyZoneTexture()
{
	if (ZoneImage == nullptr || ZoneTexture == nullptr)
	{
		return;
	}
	
	ZoneImage->SetBrushFromTexture(ZoneTexture);
}


void UMinimapNightRainZoneWidget::UpdateZone(const FVector2D& MapCenter, const FVector2D& MapRadius)
{
	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Slot))
	{
		CanvasSlot->SetAutoSize(false);
		CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		CanvasSlot->SetPosition(MapCenter);
		CanvasSlot->SetSize(MapRadius * 2.f);
	}
}

