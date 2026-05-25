// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ERNIntroBird.generated.h"

class USceneComponent;
class UNiagaraComponent;
class UCurveFloat;
class AProjectERNCharacter;

/**
 * 인트로 시퀀스용 새 액터 (ACharacter 기반).
 * - Deterministic 클라이언트 시뮬레이션: 위치 리플 OFF, Replicated 상태 변수로 동기화 후 각 머신이 동일 계산
 * - 시간은 GameState::GetServerWorldTimeSeconds()로 동기화 → 모든 머신이 동일 Alpha 계산
 * - HangPoint에 플레이어 캐릭터를 부착
 */
UCLASS()
class PROJECTERN_API AERNIntroBird : public ACharacter
{
	GENERATED_BODY()

public:
	AERNIntroBird();

	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 비행 시작 (서버 권한 — Replicated 상태 변수 세팅)
	void StartFlight();

	// 부착된 플레이어가 새에서 해제됐을 때 호출 (서버 권한) → 위로 상승 후 Destroy
	void OnPlayerReleased();

	// 캐릭터 부착 슬롯
	USceneComponent* GetHangPoint() const { return HangPoint; }

	// 부착된 플레이어 설정
	void SetAttachedPlayer(AProjectERNCharacter* Player) { AttachedPlayer = Player; }

protected:
	virtual void BeginPlay() override;

	// 캐릭터가 매달릴 슬롯 (BP에서 위치 조정, Mesh의 자식)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> HangPoint;

	// 비행 중 재생할 나이아가라 (BP에서 NiagaraSystem 에셋 할당)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UNiagaraComponent> FlightVFX;

	// === 비행 파라미터 ===

	// Forward 방향으로 비행할 거리 (cm)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Flight")
	float FlightDistance = 5000.f;

	// 시작 → 끝까지 걸리는 시간 (초)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Flight")
	float FlightDuration = 4.f;

	// === 이탈 후 ===

	// 플레이어 이탈 후 상승 지속 시간 (이후 Destroy)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FlyAway")
	float FlyAwayDuration = 3.f;

	// 이탈 후 상승 방향 (기본 Up)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FlyAway")
	FVector FlyAwayDirection = FVector::UpVector;

	// 이탈 후 상승 속도 (cm/s)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FlyAway")
	float FlyAwaySpeed = 800.f;

	// === 조향 (2D 레이싱: 직진은 그대로, 좌우 입력만 추가로 누적 이동) ===

	// 좌우 이동 속도 (cm/s) - A/D 누르고 있는 동안 매 틱 누적, 떼면 그 자리 유지
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Flight|Steering")
	float SteeringSpeed = 600.f;

public:
	// 로컬 클라가 매 입력마다 호출 - 서버에 현재 입력값(-1~1) 전달
	UFUNCTION(Server, Unreliable)
	void Server_SetSteeringInput(float Input);

private:
	// 부착된 플레이어 (Replicated)
	UPROPERTY(Replicated)
	TObjectPtr<AProjectERNCharacter> AttachedPlayer;

	// === 비행 상태 (Replicated — initial replication으로 spawn과 함께 도착) ===

	UPROPERTY(ReplicatedUsing = OnRep_IsFlying)
	bool bIsFlying = false;

	UPROPERTY(Replicated)
	FVector StartLocation = FVector::ZeroVector;

	UPROPERTY(Replicated)
	FVector FlightDirection = FVector::ForwardVector;

	// 서버 시간 기준 비행 시작 시각 (시작 시 lag 보정용 — 그 후엔 사용 안 함)
	UPROPERTY(Replicated)
	float FlightStartServerTime = 0.f;

	// 각 머신의 로컬 시간 기준 시작 시각 (Replicated 아님 — Tick에서 GetTimeSeconds()와의 차로 ElapsedTime 계산)
	float LocalFlightStartTime = 0.f;

	UFUNCTION()
	void OnRep_IsFlying();

	// === FlyAway 상태 ===

	UPROPERTY(ReplicatedUsing = OnRep_IsFlyingAway)
	bool bIsFlyingAway = false;

	UPROPERTY(Replicated)
	FVector FlyAwayStartLocation = FVector::ZeroVector;

	UPROPERTY(Replicated)
	float FlyAwayStartServerTime = 0.f;

	// 각 머신의 로컬 FlyAway 시작 시각
	float LocalFlyAwayStartTime = 0.f;

	// === 조향 상태 ===

	// 서버 권한 누적 좌우 오프셋 (모든 클라 리플 → deterministic 위치 계산)
	UPROPERTY(Replicated)
	float CurrentSteeringOffset = 0.f;

	// 서버용 현재 입력값 (-1~1) - 리플 불필요 (서버에서만 누적에 사용)
	float CurrentSteeringInput = 0.f;

	UFUNCTION()
	void OnRep_IsFlyingAway();

	// FlyAway 종료 시점에 Destroy 예약용 타이머 (서버 전용)
	FTimerHandle FlyAwayTimerHandle;

	void DestroySelf();

	// 서버 동기화된 현재 시각 반환 (GameState->GetServerWorldTimeSeconds() 래퍼)
	float GetServerNow() const;
};
