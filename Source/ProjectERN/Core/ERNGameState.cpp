// Copyright Epic Games, Inc. All Rights Reserved.

#include "Core/ERNGameState.h"
#include "GameFramework/PlayerState.h"
#include "Character/Player/ERNPlayerState.h"
#include "Net/UnrealNetwork.h"
#include "Subsystem/ERNCutsceneSubsystem.h"
#include "LevelSequence.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "Character/Enemy/ERNBossCharacter.h"
#include "Character/Player/ERNPlayerController.h"
#include "Character/Player/ProjectERNCharacter.h"
#include "Core/ERNGameInstance.h"

AERNGameState::AERNGameState()
{
	bReplicates = true;
}

void AERNGameState::StartBossEncounterSequence()
{
	if (!HasAuthority()) return;

	// 컷신 미설정 시 종료
	if (BossEncounterCutscene.IsNull())
	{
		return;
	}

	// 맵에서 보스 찾기 (보스맵엔 1마리만 있다고 가정)
	AERNBossCharacter* Boss = nullptr;
	for (TActorIterator<AERNBossCharacter> It(GetWorld()); It; ++It)
	{
		Boss = *It;
		break;
	}

	if (!Boss)
	{
		// WP 언로드 등으로 못 찾았으면 재시도 (최대 10회 = 5초)
		if (BossEncounterRetryCount < 10)
		{
			BossEncounterRetryCount++;
			FTimerHandle Tmp;
			GetWorldTimerManager().SetTimer(
				Tmp,
				this,
				&AERNGameState::StartBossEncounterSequence,
				0.5f,
				false
			);
		}
		return;
	}

	BossEncounterRetryCount = 0;
	CachedBoss = Boss;

	// 보스 AI 비활성화 (BT + Perception 정지 → 체력바 트리거 차단)
	Boss->SetIntroCutsceneLocked(true);

	// 모든 머신에서 컷신 재생
	Multicast_PlayBossEncounterCutscene();

	UE_LOG(LogTemp, Log, TEXT("[BossEncounter] Started for %s"), *Boss->GetName());
}

void AERNGameState::Multicast_PlayBossEncounterCutscene_Implementation()
{
	if (BossEncounterCutscene.IsNull())
	{
		return;
	}

	UGameInstance* GI = Cast<UGameInstance>(GetWorld()->GetGameInstance());
	if (!GI) return;

	UERNCutsceneSubsystem* CutsceneSubsystem = GI->GetSubsystem<UERNCutsceneSubsystem>();
	if (!CutsceneSubsystem) return;

	// 서버에서만 종료 콜백 바인딩 (보스 잠금 해제는 서버 권한)
	if (HasAuthority())
	{
		CutsceneSubsystem->OnCutsceneFinished.AddDynamic(this, &AERNGameState::OnBossEncounterCutsceneFinished);
	}

	CutsceneSubsystem->PlayCutscene(BossEncounterCutscene.LoadSynchronous());
}

void AERNGameState::OnBossEncounterCutsceneFinished()
{
	// 콜백 해제
	if (UGameInstance* GI = Cast<UGameInstance>(GetWorld()->GetGameInstance()))
	{
		if (UERNCutsceneSubsystem* CutsceneSubsystem = GI->GetSubsystem<UERNCutsceneSubsystem>())
		{
			CutsceneSubsystem->OnCutsceneFinished.RemoveDynamic(this, &AERNGameState::OnBossEncounterCutsceneFinished);
		}
	}

	if (!HasAuthority()) return;

	if (AERNBossCharacter* Boss = CachedBoss.Get())
	{
		// 보스 잠금 해제 → BT/Perception 재개 → 전투 시작
		Boss->SetIntroCutsceneLocked(false);
	}

	CachedBoss.Reset();
}

void AERNGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AERNGameState, CountdownTime);
	DOREPLIFETIME(AERNGameState, bIsCountingDown);
	
	//낮밤 DayNight 관련
	DOREPLIFETIME(AERNGameState, DayNightCycleState);
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

// 낮,밤 변화 관련 함수
void AERNGameState::StartDayNightCycle(float InDuration)
{
	if (HasAuthority() == false)
	{
		return;
	}
	
	DayNightCycleState.bRunning = true;
	DayNightCycleState.Duration = FMath::Max(InDuration, 0.01f);
	DayNightCycleState.StartServerWorldTimeSeconds = GetServerWorldTimeSeconds();
	DayNightCycleState.Revision++;
	
	OnRep_DayNightCycleState();
}

void AERNGameState::StopDayNightCycle()
{
	if (HasAuthority() == false)
	{
		return;
	}
	
	DayNightCycleState.bRunning = false;
	DayNightCycleState.Revision++;
	
	OnRep_DayNightCycleState();
}

void AERNGameState::OnRep_DayNightCycleState()
{
	OnDayNightCycleStateChanged.Broadcast(DayNightCycleState);
}

void AERNGameState::AddInstancePortalState(AERNPlayerState* PlayerState)
{
	if (HasAuthority() == false)
	{
		return;
	}
	
	if (IsValid(PlayerState))
	{
		InstancePortalInPlayer.Add(PlayerState);
	}
}

void AERNGameState::RemoveInstancePortalState(AERNPlayerState* PlayerState)
{
	if (HasAuthority() == false)
	{
		return;
	}
	
	if (IsValid(PlayerState))
	{
		InstancePortalInPlayer.Remove(PlayerState);
	}
}

void AERNGameState::SetActiveInstancePortalDestinationPoint(AERNPortalDestinationPoint* PortalDestinationPoint)
{
	if (HasAuthority() == false || PortalDestinationPoint == nullptr)
	{
		return;
	}
	
	ActiveInstancePortalDestinationPoint = PortalDestinationPoint;
}

AERNPortalDestinationPoint* AERNGameState::GetActiveInstancePortalDestinationPoint() const
{
	return ActiveInstancePortalDestinationPoint;
}

void AERNGameState::ClearActiveInstancePortalDestinationPoint()
{
	ActiveInstancePortalDestinationPoint = nullptr;
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
}

void AERNGameState::OnCountdownComplete()
{
	bIsCountingDown = false;
	Multicast_OnCountdownFinished();

	UE_LOG(LogTemp, Log, TEXT("Countdown complete!"));

	// 인트로 컷신이 있으면 모든 클라이언트에서 컷신 재생
	if (!IntroCutscene.IsNull())
	{
		if (UGameInstance* GI = Cast<UGameInstance>(GetWorld()->GetGameInstance()))
		{
			if (UERNCutsceneSubsystem* CutsceneSubsystem = GI->GetSubsystem<UERNCutsceneSubsystem>())
			{
				// 서버에서만 컷신 종료 콜백 바인딩 (맵 이동은 서버에서만)
				CutsceneSubsystem->OnCutsceneFinished.AddDynamic(this, &AERNGameState::OnIntroCutsceneFinished);

				// 모든 클라이언트에서 컷신 재생
				Multicast_PlayIntroCutscene();

				UE_LOG(LogTemp, Log, TEXT("Playing intro cutscene before travel"));
				return;
			}
		}
	}

	// 컷신이 없으면 바로 맵 이동
	OnIntroCutsceneFinished();
}

void AERNGameState::Multicast_PlayIntroCutscene_Implementation()
{
	if (IntroCutscene.IsNull())
	{
		return;
	}

	if (UGameInstance* GI = Cast<UGameInstance>(GetWorld()->GetGameInstance()))
	{
		if (UERNCutsceneSubsystem* CutsceneSubsystem = GI->GetSubsystem<UERNCutsceneSubsystem>())
		{
			// 클라이언트에서는 로컬 컷신 종료 콜백 바인딩 (로딩화면만 표시)
			if (!HasAuthority())
			{
				CutsceneSubsystem->OnCutsceneFinished.AddDynamic(this, &AERNGameState::OnLocalCutsceneFinished);
			}

			// 컷신 재생 (플레이어 자동 바인딩)
			CutsceneSubsystem->PlayCutscene(IntroCutscene.LoadSynchronous());
		}
	}
}

void AERNGameState::OnIntroCutsceneFinished()
{
	// 컷신 종료 콜백 해제
	if (UGameInstance* GI = Cast<UGameInstance>(GetWorld()->GetGameInstance()))
	{
		if (UERNCutsceneSubsystem* CutsceneSubsystem = GI->GetSubsystem<UERNCutsceneSubsystem>())
		{
			CutsceneSubsystem->OnCutsceneFinished.RemoveDynamic(this, &AERNGameState::OnIntroCutsceneFinished);

			// 로딩화면 호출
			CutsceneSubsystem->ShowLoadingScreen();
		}
	}

	UE_LOG(LogTemp, Log, TEXT("Traveling to field map: %s"), *FieldMapName);

	// 맵이동
	if (UWorld* World = GetWorld())
	{
		World->ServerTravel(FieldMapName + TEXT("?listen"));
	}
}

void AERNGameState::OnLocalCutsceneFinished()
{
	// 클라이언트 전용: 로딩화면만 표시 (맵 이동은 서버가 처리)
	if (UGameInstance* GI = Cast<UGameInstance>(GetWorld()->GetGameInstance()))
	{
		if (UERNCutsceneSubsystem* CutsceneSubsystem = GI->GetSubsystem<UERNCutsceneSubsystem>())
		{
			CutsceneSubsystem->OnCutsceneFinished.RemoveDynamic(this, &AERNGameState::OnLocalCutsceneFinished);

			// 로딩화면 호출
			CutsceneSubsystem->ShowLoadingScreen();
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[Client] Cutscene finished, showing loading screen"));
}

void AERNGameState::Multicast_OnCountdownFinished_Implementation()
{
	OnCountdownFinished.Broadcast();
}

void AERNGameState::HandleGameClear()
{
	EndGame(true);
}

void AERNGameState::HandleGameOver()
{
	EndGame(false);
}

void AERNGameState::TryHandleFinalZoneGameOver()
{
	if (!HasAuthority() || bEnded)
	{
		return;
	}

	if (!AreAllPlayersEliminated())
	{
		return;
	}

	if (UERNGameInstance* GI = GetGameInstance<UERNGameInstance>())
	{
		// ImmortalMode: 횟수 제한 없이 매번 전원 부활
		if (GI->IsImmortalModeEnabled())
		{
			EasyModeReviveAllPlayers();
			return;
		}

		// 1인 플레이는 EasyMode와 동일하게 런당 1회 부활 보장 (유효 플레이어 수 1명)
		int32 ValidPlayerCount = 0;
		for (const APlayerState* PS : PlayerArray)
		{
			if (IsValid(PS))
			{
				++ValidPlayerCount;
			}
		}
		const bool bSoloPlayer = (ValidPlayerCount == 1);

		// EasyMode(또는 1인 플레이): 이번 런에서 아직 안 썼으면, 패배 대신 전원 1회 부활
		if ((GI->IsEasyModeEnabled() || bSoloPlayer) && !GI->IsEasyModeReviveUsed())
		{
			GI->MarkEasyModeReviveUsed();
			EasyModeReviveAllPlayers();
			return;
		}
	}

	HandleGameOver();
}

void AERNGameState::EasyModeReviveAllPlayers()
{
	if (!HasAuthority())
	{
		return;
	}

	// 탈락한 모든 플레이어를 그자리 100% 체력으로 부활 + 성배병 2개 추가
	for (APlayerState* PS : PlayerArray)
	{
		if (!IsValid(PS))
		{
			continue;
		}

		if (AProjectERNCharacter* Character = Cast<AProjectERNCharacter>(PS->GetPawn()))
		{
			Character->ReviveForEasyMode(2);
		}
	}

	// 보스 재교전은 각 플레이어가 Alive로 복귀하는 시점(FinishRevivingState)에 트리거된다.
	// 여기서 즉시 하면 플레이어가 아직 Reviving 상태라 IsAlive() 체크에 걸려 어그로가 안 붙음.
}

bool AERNGameState::AreAllPlayersEliminated() const
{
	// 플레이어가 1명 이상 있을 때만
	if (PlayerArray.Num() == 0)
	{
		return false;
	}
	
	bool bHasValidPlayer = false;

	for (APlayerState* PS : PlayerArray)
	{
		// 유효한 PS확인
		if (!IsValid(PS))
		{
			continue;
		}

		bHasValidPlayer = true;

		// 캐릭터가 죽었는지 확인
		const AProjectERNCharacter* Character = Cast<AProjectERNCharacter>(PS->GetPawn());
		if (!IsPlayerEliminated(Character))
		{
			return false;
		}
	}

	return bHasValidPlayer;
}

void AERNGameState::EndGame(bool bVictory)
{
	if (!HasAuthority() || bEnded)
	{
		return;
	}
	bEnded = true;

	// 전과용 최종 진행/스탯 캡처 (KillCount/TotalDamage는 이미 누적돼 있음)
	for (APlayerState* PS : PlayerArray)
	{
		if (AERNPlayerState* ERNPS = Cast<AERNPlayerState>(PS))
		{
			ERNPS->SaveSnapshotFromPawn();
		}
	}

	// 전원에게 승/패 배너 표시 (배너 위젯이 일정시간 뒤 전과 위젯으로 전환)
	Multicast_ShowEndScreen(bVictory);

	// 아무도 버튼을 안 누를 경우 대비 — 타임아웃 후 강제 복귀
	GetWorldTimerManager().SetTimer(ReturnTimeoutHandle, this,
		&AERNGameState::ReturnToLobbyNow, ReturnToLobbyTimeout, false);
}

bool AERNGameState::IsPlayerEliminated(const AProjectERNCharacter* Character) const
{
	// 유효한 캐릭터인지 확인
	if (!IsValid(Character))
	{
		return false;
	}

	// 캐릭터 상태 체크
	switch (Character->GetLifeState())
	{
	case EERNPlayerLifeState::Collapsing:
	case EERNPlayerLifeState::Downed:
	case EERNPlayerLifeState::Respawning:
		return true;

	case EERNPlayerLifeState::Alive:
	case EERNPlayerLifeState::Reviving:
	default:
		return false;
	}
}

void AERNGameState::Multicast_ShowEndScreen_Implementation(bool bVictory)
{
	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		if (AERNPlayerController* ERNPC = Cast<AERNPlayerController>(PC))
		{
			ERNPC->ShowEndScreen(bVictory);
		}
	}
}

void AERNGameState::MarkReturnReady(AERNPlayerState* PS)
{
	if (!HasAuthority() || !PS)
	{
		return;
	}
	ReturnReadyPlayers.Add(PS);

	int32 ReadyCount = 0;
	for (const TWeakObjectPtr<AERNPlayerState>& Weak : ReturnReadyPlayers)
	{
		if (Weak.IsValid())
		{
			++ReadyCount;
		}
	}

	if (ReadyCount >= PlayerArray.Num())
	{
		for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
		{
			if (AERNPlayerController* PC = Cast<AERNPlayerController>(*It))
			{
				PC->Client_ResetInteractionInputState();
			}
		}
		
		ReturnToLobbyNow();
	}
}

void AERNGameState::UnmarkReturnReady(AERNPlayerState* PS)
{
	if (!HasAuthority() || !PS)
	{
		return;
	}

	ReturnReadyPlayers.Remove(PS);

	// 유효한 신청자 수 재계산
	int32 ReadyCount = 0;
	for (const TWeakObjectPtr<AERNPlayerState>& Weak : ReturnReadyPlayers)
	{
		if (Weak.IsValid())
		{
			++ReadyCount;
		}
	}

	// 신청자가 모두 취소되면 타임아웃 타이머 정지 (아무도 대기 안 함)
	if (ReadyCount == 0)
	{
		GetWorldTimerManager().ClearTimer(ReturnTimeoutHandle);
	}
}

void AERNGameState::ReturnToLobbyNow()
{
	if (!HasAuthority())
	{
		return;
	}
	GetWorldTimerManager().ClearTimer(ReturnTimeoutHandle);

	// EasyMode 1회 부활 가드 초기화 (로비 진입 = 게임 루프 종료 → 다음 런에서 다시 1회 가능)
	if (UERNGameInstance* GI = GetGameInstance<UERNGameInstance>())
	{
		GI->ResetEasyModeRunGuard();
	}

	// 진행/전과 전부 초기화 → 로비 폰은 기본값으로 스폰됨
	for (APlayerState* PS : PlayerArray)
	{
		if (AERNPlayerState* ERNPS = Cast<AERNPlayerState>(PS))
		{
			ERNPS->ClearRunProgress();
		}
	}

	// 전원에게 로딩화면 표시 (ServerTravel은 다음 틱에 이동하므로 multicast가 먼저 전달됨)
	Multicast_ShowLoadingScreen();

	if (UWorld* World = GetWorld())
	{
		World->ServerTravel(LobbyMapName + TEXT("?listen"));
	}
}

void AERNGameState::Multicast_ShowLoadingScreen_Implementation()
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UERNCutsceneSubsystem* CutsceneSubsystem = GI->GetSubsystem<UERNCutsceneSubsystem>())
		{
			CutsceneSubsystem->ShowLoadingScreen();
		}
	}
}
