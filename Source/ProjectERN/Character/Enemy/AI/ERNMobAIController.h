// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionComponent.h"
#include "ERNMobAIController.generated.h"

class UBehaviorTree;
class UBlackboardComponent;

/**
 * 일반 몬스터용 AI Controller
 */
UCLASS()
class PROJECTERN_API AERNMobAIController : public AAIController
{
	GENERATED_BODY()

public:
	AERNMobAIController();

protected:
	virtual void BeginPlay() override;
	virtual void OnPossess(APawn* InPawn) override;

	// AI Perception 설정
	void SetupPerception();

	// 타겟 감지 콜백
	UFUNCTION()
	void OnTargetDetected(AActor* Actor, FAIStimulus Stimulus);

	// 시야 설정 변경 (전투 모드 전환)
	void SetCombatVision(bool bInCombat);

protected:
	// Behavior Tree
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "AI")
	UBehaviorTree* BehaviorTreeAsset;

	// 기본 시야 거리
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Perception")
	float SightRadius = 1500.0f;

	// 시야 상실 거리
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Perception")
	float LoseSightRadius = 2000.0f;

	// 기본 시야각 (평상시)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Perception")
	float DefaultVisionAngle = 90.0f;

	// 전투 시야각 (타겟 추적 중)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Perception")
	float CombatVisionAngle = 180.0f;

	// 현재 전투 모드 여부
	bool bIsInCombatMode = false;

public:
	// Blackboard Component 접근자
	FORCEINLINE UBlackboardComponent* GetBlackboardComp() const { return Blackboard; }

	// 타겟 설정 (외부에서 호출 - TakeDamage 등)
	void SetTarget(AActor* NewTarget);
};
