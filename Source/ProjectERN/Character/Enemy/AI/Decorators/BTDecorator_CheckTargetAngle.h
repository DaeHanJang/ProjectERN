// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "BTDecorator_CheckTargetAngle.generated.h"

/**
 * 타겟이 AI 기준 특정 각도 범위에 있는지 체크하는 데코레이터
 * 보스 전방(0도) 기준 시계방향으로 MinAngle ~ MaxAngle 사이에 타겟이 있으면 true
 * 예: MinAngle=225, MaxAngle=315 → 보스 왼쪽 90도 범위 체크
 */
UCLASS()
class PROJECTERN_API UBTDecorator_CheckTargetAngle : public UBTDecorator
{
	GENERATED_BODY()

public:
	UBTDecorator_CheckTargetAngle();

protected:
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;

	// 최소 각도 (0~360, 전방 기준 시계방향)
	UPROPERTY(EditAnywhere, Category = "Condition", meta = (ClampMin = "0.0", ClampMax = "360.0"))
	float MinAngle = 90.0f;

	// 최대 각도 (0~360, 전방 기준 시계방향)
	UPROPERTY(EditAnywhere, Category = "Condition", meta = (ClampMin = "0.0", ClampMax = "360.0"))
	float MaxAngle = 270.0f;
};
