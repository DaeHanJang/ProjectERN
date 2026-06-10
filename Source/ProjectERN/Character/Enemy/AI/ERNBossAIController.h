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

	// 현재 시야에 들어와 있는 살아있는 플레이어 목록 (라이브 perception 조회 + IsAlive 필터).
	// 저장 배열 대신 라이브 조회라, 다운 후 부활한 플레이어도 즉시 다시 포함됨 (Mob과 동일 방식)
	UFUNCTION(BlueprintCallable, Category = "Boss AI")
	TArray<AActor*> GetPerceivedPlayers();

	// EasyMode 부활 후 재교전 — 살아있는 전체 플레이어를 다시 인식/어그로하여 타겟을 재획득 (서버)
	// (부활 시 perception 신규 이벤트가 안 떠서 보스가 멈추는 문제 해결)
	UFUNCTION(BlueprintCallable, Category = "Boss AI")
	void ReacquireTargetsAfterRevive();

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
	float AggroDecayRate = 0.0f;

	// 시야 안에 있을 때 초당 누적되는 기본 어그로
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss AI")
	float SightAggroPerSecond = 100.0f;

	// === 거리 기반 어그로 배율 ===
	// 가까운 거리 기준 (이 거리 이하면 NearAggroMultiplier 적용)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss AI|Distance")
	float NearDistanceThreshold = 500.f;

	// 먼 거리 기준 (이 거리 이상이면 FarAggroMultiplier 적용)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss AI|Distance")
	float FarDistanceThreshold = 2000.f;

	// 가까울 때 어그로 배율
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss AI|Distance")
	float NearAggroMultiplier = 3.0f;

	// 멀 때 어그로 배율
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss AI|Distance")
	float FarAggroMultiplier = 1.0f;

	// === 타겟 락 시간 ===
	// 타겟 변경 후 최소 유지 시간 (이 시간 동안 타겟 변경 불가)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss AI|TargetLock")
	float MinTargetLockTime = 10.0f;

	// 한 타겟 최대 유지 시간 (이 시간 초과 시 강제 변경)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss AI|TargetLock")
	float MaxTargetLockTime = 20.0f;

	// 어그로 업데이트 타이머
	FTimerHandle AggroDecayTimerHandle;
	FTimerHandle SightAggroTimerHandle;

	// 현재 시야에 들어와 있는 플레이어 목록
	UPROPERTY()
	TArray<AActor*> PerceivedPlayers;

	// 어그로 감소 처리
	void DecayAggro();

	// 시야 내 플레이어 어그로 누적 처리 (1초마다 호출)
	void TickSightAggro();

	// 거리에 따른 어그로 배율 계산
	float GetDistanceAggroMultiplier(AActor* Target) const;

	// 타겟 변경 가능 여부 체크 (최소 락 시간)
	bool CanChangeTarget() const;

	// 최대 락 시간 초과 시 강제 타겟 변경
	void CheckForceTargetChange();

	// 현재 타겟이 아닌 가장 높은 어그로 타겟 반환
	AActor* GetSecondHighestAggroTarget() const;

private:
	// 현재 타겟이 설정된 시간
	float CurrentTargetStartTime = 0.f;

	// 현재 타겟 (변경 감지용)
	UPROPERTY()
	TWeakObjectPtr<AActor> CachedCurrentTarget;
	
	// ===== 플레이어 목숨 상태에 따른 어그로 변경 헬퍼 함수 =====
	// 플레이어 캐릭터가 살아있는지 확인
	bool IsValidAggroTarget(AActor* Target) const;
	// 어그로 제거
	void RemoveAggroTarget(AActor* Target);
	// ===== 
};
