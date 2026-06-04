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
 * 매달려 날아가는 새 액터 (ACharacter 기반) — 두 가지 사용처:
 *   1) StartFlight()                — 인트로 컷신 (ERNCutsceneSubsystem에서 호출)
 *   2) StartApproachAndPickup()     — BirdStatue 이동 (Approach → Ascend → Flight 3페이즈)
 *
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

	// 비행 시작 (서버 권한 — Replicated 상태 변수 세팅) — 인트로/Forward 페이즈
	void StartFlight();

	// BirdStatue용 진입점 (서버 권한) — Approach → Ascend → Flight 자동 진행
	void StartApproachAndPickup(class AProjectERNCharacter* Target, FVector InAscentDirection);

	// BirdStatue별 비행 파라미터 주입 (서버 권한, 스폰 직후 호출).
	// Replicated 변수라 초기 리플리케이션으로 클라까지 전파 → deterministic 시뮬 일치.
	void ConfigureFlight(float InAscentHeight, float InAscentForwardDistance, float InFlightDistance, float InFlightDuration);

	// BirdStatue / 콘솔 공용: 가드 → 스폰 → Approach 시작 → 재입력 차단까지 처리.
	// bConsoleSummon=true면 빠른 비행 프로파일 적용. 실패 시 nullptr 반환.
	// (서버 권한 필요 — Target->HasAuthority() 내부 검사)
	static AERNIntroBird* RequestPickup(
		TSubclassOf<AERNIntroBird> BirdClass,
		AProjectERNCharacter* Target,
		const FTransform& SpawnXform,
		FVector Direction,
		bool bConsoleSummon);

	// 부착된 플레이어가 새에서 해제됐을 때 호출 (서버 권한) → 위로 상승 후 Destroy
	void OnPlayerReleased();

	// 캐릭터 부착 슬롯
	USceneComponent* GetHangPoint() const { return HangPoint; }

	// 부착된 플레이어 설정
	void SetAttachedPlayer(AProjectERNCharacter* Player) { AttachedPlayer = Player; }

	// 솟구침 페이즈 여부 (플레이어가 카메라 lag 토글에 사용)
	bool IsAscending() const { return bIsAscending; }

protected:
	virtual void BeginPlay() override;

	// 캐릭터가 매달릴 슬롯 (BP에서 위치 조정, Mesh의 자식)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> HangPoint;

	// HangPoint가 따라갈 본 이름 (지정 시 본의 애니메이션 delta만큼 HangPoint도 이동)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Bird")
	FName HangBoneName = NAME_None;

	// 비행 중 재생할 나이아가라 (BP에서 NiagaraSystem 에셋 할당)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UNiagaraComponent> FlightVFX;

	// === 비행 파라미터 ===

	// Forward 방향으로 비행할 거리 (cm) — BirdStatue가 인스턴스별로 덮어쓸 수 있어 Replicated
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Replicated, Category = "Flight")
	float FlightDistance = 100000.0f;

	// 시작 → 끝까지 걸리는 시간 (초) — BirdStatue가 덮어쓸 수 있어 Replicated
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Replicated, Category = "Flight")
	float FlightDuration = 50.f;

	// 콘솔 명령으로 소환된 새의 전진 비행 거리/시간 (bConsoleSummon=true면 FlightDistance/Duration 대신 사용)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Flight")
	float ConsoleFlightDistance = 100000.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Flight")
	float ConsoleFlightDuration = 50.f;

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

	// === Approach 페이즈 (BirdStatue 전용 — 플레이어를 향해 비행) ===

	// 플레이어를 향해 날아가는 속도 (cm/s)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Approach")
	float ApproachSpeed = 5000.f;

	// 플레이어 머리 위로 얼마나 위에서 도착할지 (cm) — 자연스러운 매달림 타이밍용
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Approach")
	float ApproachOverheadOffset = 100.f;

	// HangPoint와 Player 거리가 이 값 이하면 자동 attach + Ascend 진입 (cm)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Approach")
	float AttachTriggerDistance = 150.f;

	// Attach 트리거 전 카메라 widen 미리 시작할 리드 타임 (초). ETA <= 이 값이면 1회 RPC 발사.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Approach")
	float CameraPrewarmLeadTime = 0.5f;

	// === Ascend 페이즈 (BirdStatue 전용 — 솟구침) ===

	// 솟구치는 높이 (cm) — BirdStatue가 덮어쓸 수 있어 Replicated
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Replicated, Category = "Ascent")
	float AscentHeight = 8000.f;

	// 솟구치는 동안 Statue Forward 방향으로 추가 이동 거리 (cm) — BirdStatue가 덮어쓸 수 있어 Replicated
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Replicated, Category = "Ascent")
	float AscentForwardDistance = 4000.f;

	// 솟구침 지속 시간 (초)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ascent")
	float AscentDuration = 2.0f;

	// 높이 보간 곡선 (선택, 없으면 선형)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ascent")
	TObjectPtr<UCurveFloat> AscentHeightCurve = nullptr;

public:
	// 로컬 클라가 매 입력마다 호출 - 서버에 현재 입력값(-1~1) 전달
	UFUNCTION(Server, Unreliable)
	void Server_SetSteeringInput(float Input);

private:
	// 부착된 플레이어 (Replicated)
	UPROPERTY(Replicated)
	TObjectPtr<AProjectERNCharacter> AttachedPlayer;

	// 콘솔 명령으로 소환된 새인지 (Replicated → 클라도 동일하게 빠른 Alpha 계산)
	UPROPERTY(Replicated)
	bool bConsoleSummon = false;

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

	// === Approach 상태 (BirdStatue 전용) ===

	UPROPERTY(ReplicatedUsing = OnRep_IsApproaching)
	bool bIsApproaching = false;

	UPROPERTY(Replicated)
	TObjectPtr<AProjectERNCharacter> ApproachTarget;

	UFUNCTION()
	void OnRep_IsApproaching();

	// === Ascend 상태 (BirdStatue 전용) ===

	UPROPERTY(ReplicatedUsing = OnRep_IsAscending)
	bool bIsAscending = false;

	UPROPERTY(Replicated)
	FVector AscentStartLocation = FVector::ZeroVector;

	UPROPERTY(Replicated)
	FVector AscentDirection = FVector::ForwardVector;

	UPROPERTY(Replicated)
	float AscentStartServerTime = 0.f;

	// 로컬 머신 기준 Ascend 시작 시각 (lag 보정 후 Tick에서 자체 진행)
	float LocalAscentStartTime = 0.f;

	UFUNCTION()
	void OnRep_IsAscending();

	// Approach/Ascend 페이즈 전환 (서버 자동)
	void OnApproachArrived();
	void OnAscentComplete();

	// Approach 중 카메라 prewarm RPC 중복 발사 방지 (서버 transient)
	bool bCameraPrewarmTriggered = false;

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

	// === HangPoint 본 트래킹 캐싱 (BeginPlay에서 1회 설정) ===
	FVector HangPointBaseRelative = FVector::ZeroVector;
	FVector BoneRestComponentLoc = FVector::ZeroVector;
	bool bHangBoneTracking = false;
};
