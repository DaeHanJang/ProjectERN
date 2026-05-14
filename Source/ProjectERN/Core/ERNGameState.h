// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "ERNGameState.generated.h"

// 플레이어 배열 변경 시 호출되는 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlayerArrayChanged);

// 모든 플레이어 준비 완료 시 호출되는 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAllPlayersReady);

// 카운트다운 변경 시 호출되는 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCountdownChanged, int32, RemainingSeconds);

// 카운트다운 완료 (이동 시작) 시 호출되는 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCountdownFinished);

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
	UPROPERTY(BlueprintReadOnly, Category = "Countdown")
	bool bIsCountingDown = false;

	// 카운트다운 시작 (서버에서 호출)
	UFUNCTION(BlueprintCallable, Category = "Countdown")
	void StartCountdown();

	// 카운트다운 취소 (플레이어 나감 등)
	UFUNCTION(BlueprintCallable, Category = "Countdown")
	void CancelCountdown();

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_CountdownTime();

	// 카운트다운 틱 (1초마다)
	void TickCountdown();

	// 카운트다운 완료 → 맵 이동
	void OnCountdownComplete();

	FTimerHandle CountdownTimerHandle;

protected:
	// PlayerArray 리플리케이션 콜백
	virtual void AddPlayerState(APlayerState* PlayerState) override;
	virtual void RemovePlayerState(APlayerState* PlayerState) override;
};
