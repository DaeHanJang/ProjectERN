// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_Attack.generated.h"

/**
 * 근접 공격 BT Task
 */
UCLASS()
class PROJECTERN_API UBTTask_Attack : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_Attack();

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

protected:
	// 공격 애니메이션 몽타주
	UPROPERTY(EditAnywhere, Category = "Attack")
	UAnimMontage* AttackMontage;

	// 공격 쿨다운 시간
	UPROPERTY(EditAnywhere, Category = "Attack")
	float AttackCooldown = 1.5f;

	// 블랙보드 MontageDuration에 저장할 때 몽타주 길이에서 뺄 값(초)
	UPROPERTY(EditAnywhere, Category = "Attack", meta = (ClampMin = "0.0"))
	float MontageDurationOffset = 0.15f;
};
