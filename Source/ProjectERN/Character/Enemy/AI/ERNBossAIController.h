// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionComponent.h"
#include "ERNBossAIController.generated.h"

class UBehaviorTree;
class UBlackboardComponent;

/**
 * 보스 몬스터용 AI Controller
 * - 페이즈별 BT 전환
 * - 어그로 테이블 관리
 * - 패턴 선택 로직
 */
UCLASS()
class PROJECTERN_API AERNBossAIController : public AAIController
{
	GENERATED_BODY()

public:
	AERNBossAIController();

	// 비헤이비어 트리 전환 (페이즈 변경 시)
	UFUNCTION(BlueprintCallable, Category = "Boss AI")
	void SwitchBehaviorTree(UBehaviorTree* NewBehaviorTree);

	// 타겟 설정
	UFUNCTION(BlueprintCallable, Category = "Boss AI")
	void SetTarget(AActor* NewTarget);

	// 어그로 테이블에 추가/업데이트
	UFUNCTION(BlueprintCallable, Category = "Boss AI")
	void AddAggro(AActor* Target, float AggroAmount);

	// 가장 어그로가 높은 타겟 반환
	UFUNCTION(BlueprintCallable, Category = "Boss AI")
	AActor* GetHighestAggroTarget() const;

	// Blackboard Component 접근자
	FORCEINLINE UBlackboardComponent* GetBlackboardComp() const { return Blackboard; }

protected:
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;

	// AI Perception 설정
	void SetupPerception();

	// 타겟 감지 콜백
	UFUNCTION()
	void OnTargetDetected(AActor* Actor, FAIStimulus Stimulus);

protected:
	// 기본 Behavior Tree (첫 페이즈)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI")
	UBehaviorTree* DefaultBehaviorTree;

	// 현재 실행 중인 Behavior Tree
	UPROPERTY(BlueprintReadOnly, Category = "AI")
	UBehaviorTree* CurrentBehaviorTree;

	// 어그로 테이블 (Actor -> Aggro Amount)
	UPROPERTY(BlueprintReadOnly, Category = "Boss AI")
	TMap<AActor*, float> AggroTable;

	// 어그로 감소율 (초당)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss AI")
	float AggroDecayRate = 5.0f;

	// 어그로 업데이트 타이머
	FTimerHandle AggroDecayTimerHandle;

	// 어그로 감소 처리
	void DecayAggro();
};
