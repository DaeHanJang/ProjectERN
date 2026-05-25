// Fill out your copyright notice in the Description page of Project Settings.


#include "ERNMinimapPinPoint.h"

#include "Net/UnrealNetwork.h"


// Sets default values
AERNMinimapPinPoint::AERNMinimapPinPoint()
{
	IconType = EERNMinimapIconType::PlayerPin1;
	bVisibleOnMinimap = true;
	IconScale = 1.0f;
	bIsSpatiallyLoaded = false;
}

void AERNMinimapPinPoint::InitializePin( EERNMinimapIconType InIconType, APlayerState* InOwnerPlayerState)
{
	if (HasAuthority() == false)
	{
		return;
	}
	
	IconType = InIconType;
	OwnerPlayerState = InOwnerPlayerState;
	bVisibleOnMinimap = true;

	NotifyMinimapChanged();
}

void AERNMinimapPinPoint::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(AERNMinimapPinPoint, OwnerPlayerState);
}

