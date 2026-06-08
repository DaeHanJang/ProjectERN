// Copyright Epic Games, Inc. All Rights Reserved.

#include "Actors/ERNWatcherTeleportPoint.h"
#include "Components/ArrowComponent.h"
#include "Components/SceneComponent.h"

AERNWatcherTeleportPoint::AERNWatcherTeleportPoint()
{
	// 텔레포트 지점은 매 틱 업데이트될 필요가 없으므로 성능을 위해 false 처리
	PrimaryActorTick.bCanEverTick = false;

	DefaultRootComp = CreateDefaultSubobject<USceneComponent>(TEXT("DefaultRootComp"));
	SetRootComponent(DefaultRootComp);

#if WITH_EDITORONLY_DATA
	// 에디터 상에서 배치할 때 방향을 쉽게 잡을 수 있도록 화살표 컴포넌트 추가
	ArrowComp = CreateDefaultSubobject<UArrowComponent>(TEXT("ArrowComp"));
	ArrowComp->SetupAttachment(RootComponent);
	ArrowComp->ArrowColor = FColor::Red;
	ArrowComp->ArrowSize = 1.5f;
#endif
}
