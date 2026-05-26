// Fill out your copyright notice in the Description page of Project Settings.


#include "MinimapPlayerMarkerWidget.h"

#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "GameFramework/Pawn.h"


void UMinimapPlayerMarkerWidget::SetPlayerPawn(APawn* InPawn)
{
	if (InPawn == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("InPawn is nullptr"));
		return;
	}
	
	SourcePawn = InPawn;
}

void UMinimapPlayerMarkerWidget::UpdateMarker(const FVector2D& MapPosition, float YawDegrees)
{
	if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Slot))
	{
		CanvasSlot->SetPosition(MapPosition);
		CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
	}
	
	SetRenderTransformPivot(FVector2D(0.5, 0.5f));
	SetRenderTransformAngle(YawDegrees);
}

void UMinimapPlayerMarkerWidget::SetIconTexture(UTexture2D* IconTexture)
{
	if (IconImage && IconTexture)
	{
		IconImage->SetBrushFromTexture(IconTexture);
	}
}
