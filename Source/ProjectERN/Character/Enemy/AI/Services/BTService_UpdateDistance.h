// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTService.h"
#include "BTService_UpdateDistance.generated.h"

/**
 * TargetActor와의 거리를 계산하여 블랙보드에 저장하는 서비스
 * Blackboard Decorator + Observer Aborts와 함께 사용하여 거리 기반 분기 구현
 */
UCLASS()
class PROJECTERN_API UBTService_UpdateDistance : public UBTService
{
	GENERATED_BODY()

public:
	UBTService_UpdateDistance();

protected:
	virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

	// 거리를 계산할 타겟의 블랙보드 키
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector TargetKey;

	// 거리를 저장할 블랙보드 키
	UPROPERTY(EditAnywhere, Category = "Blackboard")
	FBlackboardKeySelector DistanceKey;
};
