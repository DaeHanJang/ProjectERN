// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_LevitateBehavior.generated.h"

class AERNEnemyCharacter;
class UAnimMontage;

/**
 * 공중 부양 패턴 BT Task (latent)
 * 상승 시작과 동시에 몽타주 재생 → 지정 높이에서 공중 정지 → 몽타주 종료 후 하강 → 착지 시 태스크 종료
 * - MOVE_Flying으로 중력을 끄고 서버에서 Z 보간 (위치는 리플리케이션으로 클라 동기화)
 * - 진행 상태를 멤버에 저장하므로 bCreateNodeInstance 사용
 */
UCLASS()
class PROJECTERN_API UBTTask_LevitateBehavior : public UBTTaskNode
{
	GENERATED_BODY()

public:
	UBTTask_LevitateBehavior();

protected:
	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;

	// 공중 패턴 몽타주 (상승 시작과 동시에 재생)
	UPROPERTY(EditAnywhere, Category = "Levitate")
	UAnimMontage* Montage = nullptr;

	// 띄울 높이 (시작 위치 기준, cm)
	UPROPERTY(EditAnywhere, Category = "Levitate", meta = (ClampMin = "0.0"))
	float LevitateHeight = 1500.f;

	// 상승 속도 (cm/s)
	UPROPERTY(EditAnywhere, Category = "Levitate", meta = (ClampMin = "1.0"))
	float RiseSpeed = 1000.f;

	// 하강 속도 (cm/s)
	UPROPERTY(EditAnywhere, Category = "Levitate", meta = (ClampMin = "1.0"))
	float DescendSpeed = 800.f;

	// 몽타주 종료 후 추가 공중 체류 시간 (초)
	UPROPERTY(EditAnywhere, Category = "Levitate", meta = (ClampMin = "0.0"))
	float ExtraHoverTime = 0.f;

private:
	// 진행 페이즈
	enum class ELevitatePhase : uint8
	{
		None,
		Rise,
		Hover,
		Descend
	};

	ELevitatePhase Phase = ELevitatePhase::None;
	FVector StartLocation = FVector::ZeroVector;
	bool bMontageFinished = false;
	float HoverRemaining = 0.f;

	TWeakObjectPtr<AERNEnemyCharacter> CachedEnemy;

	// 몽타주 종료 콜백 (서버 AnimInstance)
	void OnMontageEnded(UAnimMontage* EndedMontage, bool bInterrupted);

	// 이동 모드 복원 (착지/중단 공통)
	void RestoreMovement();
};
