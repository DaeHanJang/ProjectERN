// Fill out your copyright notice in the Description page of Project Settings.


#include "ERNMinimapPinPoint.h"

#include "Net/UnrealNetwork.h"


// Sets default values
AERNMinimapPinPoint::AERNMinimapPinPoint()
{
	IconType = EERNMinimapIconType::PlayerPin1;
	bVisibleOnMinimap = true;
	IconScale = 1.0f;
	//패키징 빌드에서 설정할 수 없어서 BP에서 직접 수정함
	//bIsSpatiallyLoaded = false;
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

	// 서버측 변경
	ApplyPinColorByIconType(IconType);
	
	NotifyMinimapChanged();
}

void AERNMinimapPinPoint::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(AERNMinimapPinPoint, OwnerPlayerState);
}

void AERNMinimapPinPoint::OnRep_MinimapMarkerData()
{
	Super::OnRep_MinimapMarkerData();
	
	// 클라이언트측 변경
	ApplyPinColorByIconType(IconType);
}

