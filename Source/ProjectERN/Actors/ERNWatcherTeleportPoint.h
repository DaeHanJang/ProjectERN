// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ERNWatcherTeleportPoint.generated.h"

class UArrowComponent;
class USceneComponent;

/**
 * 감시 몬스터에게 발각되었을 때 플레이어가 강제 순간이동되는 목적지 포인트 액터
 */
UCLASS()
class PROJECTERN_API AERNWatcherTeleportPoint : public AActor
{
	GENERATED_BODY()
	
public:	
	AERNWatcherTeleportPoint();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USceneComponent> DefaultRootComp;

#if WITH_EDITORONLY_DATA
	// 에디터에서 텔레포트 후 플레이어가 바라볼 방향을 시각적으로 확인하기 위한 화살표
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UArrowComponent> ArrowComp;
#endif
};
