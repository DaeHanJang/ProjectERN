// Fill out your copyright notice in the Description page of Project Settings.


#include "ERNMinimapTargetPoint.h"

#include "ERNMinimapSubsystem.h"
#include "Net/UnrealNetwork.h"

AERNMinimapTargetPoint::AERNMinimapTargetPoint()
{
	// 타겟 포인트 복제 가능하도록 생성
	bReplicates = true;
	bAlwaysRelevant = true;
	AActor::SetReplicateMovement(false);
}

void AERNMinimapTargetPoint::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(AERNMinimapTargetPoint, IconType);
	DOREPLIFETIME(AERNMinimapTargetPoint, bVisibleOnMinimap);
	DOREPLIFETIME(AERNMinimapTargetPoint, IconScale);
}

void AERNMinimapTargetPoint::BeginPlay()
{
	Super::BeginPlay();

	if (bVisibleOnMinimap)
	{
		RegisterToMinimapSubsystem();
	}
}

void AERNMinimapTargetPoint::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnregisterFromMinimapSubsystem();
	Super::EndPlay(EndPlayReason);
}

void AERNMinimapTargetPoint::OnRep_MinimapMarkerData()
{
	if (bVisibleOnMinimap)
	{
		RegisterToMinimapSubsystem();
	}
	else
	{
		UnregisterFromMinimapSubsystem();
	}

	NotifyMinimapChanged();
}

void AERNMinimapTargetPoint::RegisterToMinimapSubsystem()
{
	if (UERNMinimapSubsystem* Subsystem = GetWorld() ? GetWorld()->GetSubsystem<UERNMinimapSubsystem>() : nullptr)
	{
		Subsystem->RegisterTarget(this);
	}
}

void AERNMinimapTargetPoint::UnregisterFromMinimapSubsystem()
{
	if (UERNMinimapSubsystem* Subsystem = GetWorld() ? GetWorld()->GetSubsystem<UERNMinimapSubsystem>() : nullptr)
	{
		Subsystem->UnregisterTarget(this);
	}
}

void AERNMinimapTargetPoint::NotifyMinimapChanged() const
{
	if (UERNMinimapSubsystem* Subsystem = GetWorld() ? GetWorld()->GetSubsystem<UERNMinimapSubsystem>() : nullptr)
	{
		Subsystem->NotifyTargetsChanged();
	}
}
