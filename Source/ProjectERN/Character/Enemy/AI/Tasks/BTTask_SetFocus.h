// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_SetFocus.generated.h"

/**
 * AIController의 Focus를 설정하거나 해제하는 태스크
 */
UCLASS()
class PROJECTERN_API UBTTask_SetFocus : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_SetFocus();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	// Focus를 설정할 타겟의 블랙보드 키
	UPROPERTY(EditAnywhere, Category = "Focus")
	FBlackboardKeySelector TargetKey;

	// true: SetFocus, false: ClearFocus
	UPROPERTY(EditAnywhere, Category = "Focus")
	bool bSetFocus;
};
