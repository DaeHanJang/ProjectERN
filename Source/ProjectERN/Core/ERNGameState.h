// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "World/Data/ERNDayNightCycleState.h"
#include "ERNGameState.generated.h"

class ULevelSequence;
class AERNPlayerState;
class AProjectERNCharacter;

// 플레이어 배열 변경 시 호출되는 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlayerArrayChanged);

// 모든 플레이어 준비 완료 시 호출되는 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAllPlayersReady);

// 카운트다운 변경 시 호출되는 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCountdownChanged, int32, RemainingSeconds);

// 카운트다운 완료 (이동 시작) 시 호출되는 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCountdownFinished);

// 낮,밤 변화시 호출되는 델리게이트
DECLARE_MULTICAST_DELEGATE_OneParam(FOnDayNightCycleStateChanged, const FERNDayNightCycleState&);

UCLASS()
class PROJECTERN_API AERNGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	AERNGameState();

	// 플레이어 배열 변경 이벤트
	UPROPERTY(BlueprintAssignable, Category = "GameState")
	FOnPlayerArrayChanged OnPlayerArrayChanged;

	// 모든 플레이어 준비 완료 이벤트
	UPROPERTY(BlueprintAssignable, Category = "GameState")
	FOnAllPlayersReady OnAllPlayersReady;

	// 모든 플레이어 준비 상태 확인 (서버에서만 호출)
	void CheckAllPlayersReady();

	// 필드맵 이름 (블루프린트에서 설정 가능, 예: "Map_Field")
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game")
	FString FieldMapName = TEXT("Map_Field");

	// 로비 인트로 컷신 (카운트다운 완료 후 재생)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cutscene")
	TSoftObjectPtr<ULevelSequence> IntroCutscene;

	// 보스 조우 컷신 (보스맵 BeginPlay 시 자동 재생)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Cutscene")
	TSoftObjectPtr<ULevelSequence> BossEncounterCutscene;

	// === 카운트다운 시스템 ===

	// 카운트다운 변경 이벤트 (UI 바인딩용)
	UPROPERTY(BlueprintAssignable, Category = "Countdown")
	FOnCountdownChanged OnCountdownChanged;

	// 카운트다운 완료 이벤트
	UPROPERTY(BlueprintAssignable, Category = "Countdown")
	FOnCountdownFinished OnCountdownFinished;

	// 카운트다운 시작 시간 (초)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Countdown")
	int32 CountdownDuration = 5;

	// 현재 남은 카운트다운 시간 (리플리케이트)
	UPROPERTY(ReplicatedUsing = OnRep_CountdownTime, BlueprintReadOnly, Category = "Countdown")
	int32 CountdownTime = 0;

	// 카운트다운 진행 중 여부
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Countdown")
	bool bIsCountingDown = false;

	// 카운트다운 시작 (서버에서 호출)
	UFUNCTION(BlueprintCallable, Category = "Countdown")
	void StartCountdown();

	// 카운트다운 취소 (플레이어 나감 등)
	UFUNCTION(BlueprintCallable, Category = "Countdown")
	void CancelCountdown();

	// 보스 조우 컷신 시작 — 트리거 박스(AERNBossCutsceneTrigger)에서 호출
	UFUNCTION(BlueprintCallable, Category = "Boss")
	void StartBossEncounterSequence();

	// === 게임 종료(승/패) → 전과 → 로비 ===
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game")
	FString LobbyMapName = TEXT("Map_Lobby");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Game")
	float ReturnToLobbyTimeout = 60.f;

	// 승리 (최종보스 사망 시 ERNBossCharacter가 호출)
	UFUNCTION(BlueprintCallable, Category = "Game")
	void HandleGameClear();

	// 패배 (패배 조건 확정 시 호출)
	UFUNCTION(BlueprintCallable, Category = "Game")
	void HandleGameOver();

	// 최종 자기장 수렴 이후 모든 플레이어가 탈락 상태인지 확인하고 GameOver를 실행
	UFUNCTION(BlueprintCallable, Category = "Game")
	void TryHandleFinalZoneGameOver();

	// 최종 자기장 기준 전원 탈락 여부
	UFUNCTION(BlueprintPure, Category = "Game")
	bool AreAllPlayersEliminated() const;
	
	// 전과 위젯 "로비로" 버튼 → PC RPC가 호출 (복귀 준비 등록)
	void MarkReturnReady(AERNPlayerState* PS);

	// 전과 위젯 "취소" 버튼 → PC RPC가 호출 (복귀 신청 해제)
	void UnmarkReturnReady(AERNPlayerState* PS);

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_CountdownTime();

	// 모든 머신에서 보스 조우 컷신 재생
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayBossEncounterCutscene();

	// 보스 조우 컷신 종료 콜백 (서버: 보스 잠금 해제 + 체력바 표시)
	UFUNCTION()
	void OnBossEncounterCutsceneFinished();

	// 보스 검색 재시도 횟수 (WP 언로드 등 대응)
	int32 BossEncounterRetryCount = 0;

	// 현재 컷신에 사용 중인 보스 참조 (종료 콜백에서 사용)
	UPROPERTY()
	TWeakObjectPtr<class AERNBossCharacter> CachedBoss;

	// 카운트다운 틱 (1초마다)
	void TickCountdown();

	// 카운트다운 완료 → 컷신 재생 또는 맵 이동
	void OnCountdownComplete();

	// 모든 클라이언트에서 컷신 재생
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayIntroCutscene();

	// 카운트다운 완료 알림 (모든 클라이언트)
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_OnCountdownFinished();

	// 인트로 컷신 종료 콜백 (서버: 로딩화면 + 맵이동)
	UFUNCTION()
	void OnIntroCutsceneFinished();

	// 로컬 컷신 종료 콜백 (클라이언트: 로딩화면만)
	UFUNCTION()
	void OnLocalCutsceneFinished();

	FTimerHandle CountdownTimerHandle;

	// === 게임 종료 처리 내부 ===
	void EndGame(bool bVictory);

	// 캐릭터 상태를 확인하여 반환
	bool IsPlayerEliminated(const AProjectERNCharacter* Character) const;
	
	// 전원에게 승/패 배너 표시 (배너 위젯이 일정시간 뒤 전과 위젯으로 전환)
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_ShowEndScreen(bool bVictory);

	// 전원(호스트+클라)에게 로딩화면 표시 — 로비 복귀 ServerTravel 직전 호출
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_ShowLoadingScreen();

	// 진행/전과 초기화 후 로비로 ServerTravel
	void ReturnToLobbyNow();

	UPROPERTY()
	TSet<TWeakObjectPtr<AERNPlayerState>> ReturnReadyPlayers;

	bool bEnded = false;
	FTimerHandle ReturnTimeoutHandle;

protected:
	// PlayerArray 리플리케이션 콜백
	virtual void AddPlayerState(APlayerState* PlayerState) override;
	virtual void RemovePlayerState(APlayerState* PlayerState) override;
	
#pragma region DayNightCycle
public:
	const FERNDayNightCycleState& GetDayNightCycleState() const { return DayNightCycleState; }
	void StartDayNightCycle(float InDuration);
	void StopDayNightCycle();
	
	// 낮->밤 상태 변화
	FOnDayNightCycleStateChanged OnDayNightCycleStateChanged;
	
protected:
	UFUNCTION()
	void OnRep_DayNightCycleState();
	
private:
	UPROPERTY(ReplicatedUsing = OnRep_DayNightCycleState)
	FERNDayNightCycleState DayNightCycleState;
#pragma endregion DayNightCycle
	
#pragma region InstancePortal
public:
	//  인스턴스 포탈 사용중인 플레이어 증가
	void AddInstancePortalState(AERNPlayerState* PlayerState);
	//  인스턴스 포탈 사용중인 플레이어 감소
	void RemoveInstancePortalState(AERNPlayerState* PlayerState);
	//  인스턴스 포탈 사용중인 플레이어 수
	int32 GetInstancePortalInPlayer() const { return InstancePortalInPlayer.Num(); }
	
private:
	
	//  인스턴스 포탈 사용중인 플레이어
	UPROPERTY()
	TSet<TObjectPtr<AERNPlayerState>> InstancePortalInPlayer;
#pragma endregion InstancePortal
};
