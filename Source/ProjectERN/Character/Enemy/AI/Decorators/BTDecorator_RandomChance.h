// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTDecorator.h"
#include "BTDecorator_RandomChance.generated.h"

/**
 * 확률 기반 BT 노드 실행 데코레이터
 * 설정한 확률(0~100%)에 따라 노드 실행 여부 결정
 */
UCLASS()
class PROJECTERN_API UBTDecorator_RandomChance : public UBTDecorator
{
	GENERATED_BODY()

public:
	UBTDecorator_RandomChance();

protected:
	virtual bool CalculateRawConditionValue(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) const override;

	// 실행 확률 (0~100)
	UPROPERTY(EditAnywhere, Category = "Condition", meta = (ClampMin = "0.0", ClampMax = "100.0"))
	float Chance;
};
