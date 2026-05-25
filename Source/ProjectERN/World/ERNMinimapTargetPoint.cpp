// Fill out your copyright notice in the Description page of Project Settings.


#include "ERNMinimapTargetPoint.h"

#include "ERNMinimapSubsystem.h"

void AERNMinimapTargetPoint::BeginPlay()
{
	Super::BeginPlay();

	if (bVisibleOnMinimap)
	{
		if (UERNMinimapSubsystem* Subsystem = GetWorld()->GetSubsystem<UERNMinimapSubsystem>())
		{
			Subsystem->RegisterTarget(this);
		}
	}
}

void AERNMinimapTargetPoint::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UERNMinimapSubsystem* Subsystem = GetWorld() ? GetWorld()->GetSubsystem<UERNMinimapSubsystem>() : nullptr)
	{
		Subsystem->UnregisterTarget(this);
	}

	Super::EndPlay(EndPlayReason);
}