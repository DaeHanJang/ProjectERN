// Copyright Epic Games, Inc. All Rights Reserved.

#include "Core/ERNGameState.h"
#include "GameFramework/PlayerState.h"
#include "Character/Player/ERNPlayerState.h"
#include "Net/UnrealNetwork.h"

AERNGameState::AERNGameState()
{
	bReplicates = true;
}

void AERNGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AERNGameState, CountdownTime);
}

void AERNGameState::AddPlayerState(APlayerState* PlayerState)
{
	Super::AddPlayerState(PlayerState);

	// 플레이어가 추가되면 델리게이트 브로드캐스트
	UE_LOG(LogTemp, Log, TEXT("Player added to GameState. Total players: %d"), PlayerArray.Num());
	OnPlayerArrayChanged.Broadcast();
}

void AERNGameState::RemovePlayerState(APlayerState* PlayerState)
{
	Super::RemovePlayerState(PlayerState);

	// 플레이어가 제거되면 델리게이트 브로드캐스트
	UE_LOG(LogTemp, Log, TEXT("Player removed from GameState. Total players: %d"), PlayerArray.Num());
	OnPlayerArrayChanged.Broadcast();
}

void AERNGameState::CheckAllPlayersReady()
{
	// 서버에서만 실행
	if (!HasAuthority())
	{
		return;
	}

	// 플레이어가 없으면 체크하지 않음
	if (PlayerArray.Num() == 0)
	{
		return;
	}

	// 모든 플레이어가 준비됐는지 확인
	bool bAllReady = true;
	for (APlayerState* PS : PlayerArray)
	{
		if (AERNPlayerState* ERNPlayerState = Cast<AERNPlayerState>(PS))
		{
			if (!ERNPlayerState->bIsReady)
			{
				bAllReady = false;
				UE_LOG(LogTemp, Log, TEXT("Player %s is not ready yet"), *ERNPlayerState->PlayerNickname);
				break;
			}
		}
	}

	// 모두 준비되면 카운트다운 시작
	if (bAllReady && !bIsCountingDown)
	{
		UE_LOG(LogTemp, Log, TEXT("All players ready! Starting countdown..."));
		OnAllPlayersReady.Broadcast();
		StartCountdown();
	}
}

void AERNGameState::StartCountdown()
{
	if (!HasAuthority() || bIsCountingDown)
	{
		return;
	}

	bIsCountingDown = true;
	CountdownTime = CountdownDuration;

	// 즉시 클라이언트에 알림
	OnRep_CountdownTime();

	// 1초마다 틱
	GetWorldTimerManager().SetTimer(
		CountdownTimerHandle,
		this,
		&AERNGameState::TickCountdown,
		1.0f,
		true
	);
}

void AERNGameState::CancelCountdown()
{
	if (!HasAuthority())
	{
		return;
	}

	bIsCountingDown = false;
	CountdownTime = 0;
	GetWorldTimerManager().ClearTimer(CountdownTimerHandle);

	// 클라이언트에 취소 알림
	OnRep_CountdownTime();
}

void AERNGameState::TickCountdown()
{
	if (!HasAuthority())
	{
		return;
	}

	CountdownTime--;

	// 서버에서도 UI 업데이트 (OnRep은 클라이언트에서만 자동 호출됨)
	OnCountdownChanged.Broadcast(CountdownTime);

	if (CountdownTime <= 0)
	{
		GetWorldTimerManager().ClearTimer(CountdownTimerHandle);
		OnCountdownComplete();
	}
}

void AERNGameState::OnRep_CountdownTime()
{
	// 모든 클라이언트에서 UI 업데이트
	OnCountdownChanged.Broadcast(CountdownTime);

	if (CountdownTime <= 0 && bIsCountingDown)
	{
		OnCountdownFinished.Broadcast();
	}
}

void AERNGameState::OnCountdownComplete()
{
	bIsCountingDown = false;
	OnCountdownFinished.Broadcast();

	UE_LOG(LogTemp, Log, TEXT("Countdown complete! Traveling to field map: %s"), *FieldMapName);

	if (UWorld* World = GetWorld())
	{
		World->ServerTravel(FieldMapName + TEXT("?listen"));
	}
}
