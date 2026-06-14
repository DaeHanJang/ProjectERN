// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/TargetPoint.h"
#include "ERNPortalDestinationPoint.generated.h"

class UWorldPartitionStreamingSourceComponent;
/**
 * 인스턴스 포탈의 이동 목표로 잡을 후보 지점
 * 레벨에 여러 개 배치하고, 적 사망 시 빈(bIsUsed==false) 지점 하나를 골라 포탈을 스폰
 * 모든 포탈이 일회용이므로 한 번 점유된 지점은 반납 x (영구 소진)
 * TActorIterator 로만 순회하여 탐색 비용을 최소화
 */
UCLASS()
class PROJECTERN_API AERNPortalDestinationPoint : public ATargetPoint
{
	GENERATED_BODY()

public:
	AERNPortalDestinationPoint();
	
	// 이미 포탈이 스폰된 지점인지 여부 (서버 권위, 복제 불필요)
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Portal")
	bool bIsUsed = false;
	
	// 안정적인 지형 로드를 위한 Streamin Source
	void EnableDungeonStreamingSource();
	void DisableDungeonStreamingSource();
	
private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Portal", meta = (AllowPrivateAccess = true))
	TObjectPtr<UWorldPartitionStreamingSourceComponent> StreamingSourceComponent;
};
