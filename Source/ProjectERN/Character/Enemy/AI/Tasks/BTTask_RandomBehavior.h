// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_RandomBehavior.generated.h"

// 가중치를 가진 몽타주 정보
USTRUCT(BlueprintType)
struct FWeightedMontage
{
	GENERATED_BODY()

	// 재생할 몽타주
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimMontage* Montage = nullptr;

	// 가중치
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0.0"))
	float Weight = 1.0f;
};

/**
 * 가중치 기반으로 랜덤하게 몽타주를 선택하여 재생하는 태스크
 * 보스 패턴 선택 등에 사용
 */
UCLASS()
class PROJECTERN_API UBTTask_RandomBehavior : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_RandomBehavior();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	// 가중치를 고려한 랜덤 몽타주 배열
	UPROPERTY(EditAnywhere, Category = "Behavior")
	TArray<FWeightedMontage> WeightedMontages;

	// 블랙보드 MontageDuration에 저장할 때 몽타주 길이에서 뺄 값(초)
	UPROPERTY(EditAnywhere, Category = "Behavior", meta = (ClampMin = "0.0"))
	float MontageDurationOffset = 0.15f;

private:
	// 가중치 기반 랜덤 선택
	UAnimMontage* SelectRandomMontage() const;
};
