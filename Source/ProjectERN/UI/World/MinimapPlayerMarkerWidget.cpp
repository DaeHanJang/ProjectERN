// Fill out your copyright notice in the Description page of Project Settings.


#include "MinimapPlayerMarkerWidget.h"
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
	SetRenderTranslation(MapPosition);
	SetRenderTransformAngle(YawDegrees);
}