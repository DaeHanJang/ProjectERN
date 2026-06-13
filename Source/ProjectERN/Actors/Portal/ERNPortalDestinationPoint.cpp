// Copyright Epic Games, Inc. All Rights Reserved.

#include "Actors/Portal/ERNPortalDestinationPoint.h"

#include "Components/WorldPartitionStreamingSourceComponent.h"

AERNPortalDestinationPoint::AERNPortalDestinationPoint()
{
	//스트리밍 소스 컴포넌트 설정
	StreamingSourceComponent = CreateDefaultSubobject<UWorldPartitionStreamingSourceComponent>(TEXT("StreamingSourceComponent"));
	StreamingSourceComponent->DisableStreamingSource();
}

void AERNPortalDestinationPoint::EnableDungeonStreamingSource()
{
	if (StreamingSourceComponent)
	{
		StreamingSourceComponent->EnableStreamingSource();
	}
}

void AERNPortalDestinationPoint::DisableDungeonStreamingSource()
{
	if (StreamingSourceComponent)
	{
		StreamingSourceComponent->DisableStreamingSource();
	}
}