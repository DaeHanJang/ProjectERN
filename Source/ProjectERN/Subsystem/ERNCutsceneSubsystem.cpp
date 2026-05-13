// Copyright Epic Games, Inc. All Rights Reserved.

#include "Subsystem/ERNCutsceneSubsystem.h"
#include "Blueprint/UserWidget.h"
#include "Engine/GameViewportClient.h"
#include "Kismet/GameplayStatics.h"
#include "LevelSequence.h"
#include "LevelSequencePlayer.h"
#include "LevelSequenceActor.h"
#include "MovieSceneSequencePlayer.h"
#include "Core/ERNGameInstance.h"
#include "Widgets/SWeakWidget.h"

void UERNCutsceneSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// 로딩 화면 델리게이트 바인딩
	FCoreUObjectDelegates::PreLoadMap.AddUObject(this, &UERNCutsceneSubsystem::OnPreLoadMap);
	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UERNCutsceneSubsystem::OnPostLoadMapWithWorld);
}

void UERNCutsceneSubsystem::Deinitialize()
{
	// 델리게이트 해제
	FCoreUObjectDelegates::PreLoadMap.RemoveAll(this);
	FCoreUObjectDelegates::PostLoadMapWithWorld.RemoveAll(this);

	// 위젯 정리
	if (LoadingWidget)
	{
		LoadingWidget->RemoveFromParent();
		LoadingWidget = nullptr;
	}

	Super::Deinitialize();
}

// ===== 로딩 화면 시스템 =====

void UERNCutsceneSubsystem::OnPreLoadMap(const FString& MapName)
{
	ShowLoadingScreen();
}

void UERNCutsceneSubsystem::OnPostLoadMapWithWorld(UWorld* LoadedWorld)
{
	if (LoadedWorld)
	{
		FTimerHandle TimerHandle;
		LoadedWorld->GetTimerManager().SetTimer(
			TimerHandle,
			this,
			&UERNCutsceneSubsystem::HideLoadingScreen,
			5.0f,
			false
		);
	}
	else
	{
		HideLoadingScreen();
	}
}

void UERNCutsceneSubsystem::ShowLoadingScreen()
{
	UE_LOG(LogTemp, Warning, TEXT("[CutsceneSubsystem] ShowLoadingScreen called. bIsLoading=%d"), bIsLoading);

	if (bIsLoading)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CutsceneSubsystem] Already loading, skip"));
		return;
	}

	bIsLoading = true;
	OnLoadingStarted.Broadcast();

	// GameInstance에서 LoadingWidgetClass 가져오기
	UERNGameInstance* GameInst = Cast<UERNGameInstance>(GetGameInstance());
	if (!GameInst)
	{
		UE_LOG(LogTemp, Error, TEXT("[CutsceneSubsystem] GameInstance cast failed!"));
		return;
	}

	if (!GameInst->GetLoadingWidgetClass())
	{
		UE_LOG(LogTemp, Error, TEXT("[CutsceneSubsystem] LoadingWidgetClass not set in GameInstance!"));
		return;
	}

	UGameViewportClient* ViewportClient = GetGameInstance()->GetGameViewportClient();
	if (!ViewportClient)
	{
		UE_LOG(LogTemp, Error, TEXT("[CutsceneSubsystem] GameViewportClient is null!"));
		return;
	}

	// GameInstance를 소유자로 위젯 생성 (PC 파괴와 무관하게 유지)
	LoadingWidget = CreateWidget<UUserWidget>(GetGameInstance(), GameInst->GetLoadingWidgetClass());
	if (LoadingWidget)
	{
		// 낮은 레벨의 Slate 위젯으로 추가 (맵 전환에도 유지됨)
		TSharedRef<SWidget> SlateWidget = LoadingWidget->TakeWidget();
		ViewportClient->AddViewportWidgetContent(SlateWidget, 9999);
		LoadingSlateWidget = SlateWidget;
		UE_LOG(LogTemp, Log, TEXT("[CutsceneSubsystem] Loading widget added via AddViewportWidgetContent!"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[CutsceneSubsystem] Failed to create loading widget!"));
	}
}

void UERNCutsceneSubsystem::HideLoadingScreen()
{
	if (!bIsLoading)
	{
		return;
	}

	bIsLoading = false;

	// Slate 위젯 제거
	if (LoadingSlateWidget.IsValid())
	{
		UGameViewportClient* ViewportClient = GetGameInstance()->GetGameViewportClient();
		if (ViewportClient)
		{
			ViewportClient->RemoveViewportWidgetContent(LoadingSlateWidget.ToSharedRef());
		}
		LoadingSlateWidget.Reset();
	}

	if (LoadingWidget)
	{
		LoadingWidget = nullptr;
	}

	OnLoadingFinished.Broadcast();
}

// ===== 컷신 시스템 =====

void UERNCutsceneSubsystem::PlayCutscene(ULevelSequence* Sequence, bool bDisablePlayerInput)
{
	if (!Sequence)
	{
		return;
	}

	// 이미 재생 중이면 중단
	if (bIsPlayingCutscene)
	{
		StopCutscene();
	}

	bIsPlayingCutscene = true;
	bInputDisabledDuringCutscene = bDisablePlayerInput;
	OnCutsceneStarted.Broadcast();

	// 플레이어 입력 차단
	if (bDisablePlayerInput)
	{
		DisablePlayerInput();
	}

	// 시퀀스 플레이어 생성
	UWorld* World = GetGameInstance()->GetWorld();
	if (World)
	{
		FMovieSceneSequencePlaybackSettings PlaybackSettings;
		CurrentSequencePlayer = ULevelSequencePlayer::CreateLevelSequencePlayer(
			World, Sequence, PlaybackSettings, CurrentSequenceActor);

		if (CurrentSequencePlayer)
		{
			// 종료 콜백 바인딩
			CurrentSequencePlayer->OnFinished.AddDynamic(this, &UERNCutsceneSubsystem::OnCutsceneComplete);

			// 재생 시작
			CurrentSequencePlayer->Play();
		}
	}
}

void UERNCutsceneSubsystem::StopCutscene()
{
	if (!bIsPlayingCutscene)
	{
		return;
	}

	// 시퀀스 중단
	if (CurrentSequencePlayer)
	{
		CurrentSequencePlayer->Stop();
		CurrentSequencePlayer = nullptr;
	}

	CurrentSequenceActor = nullptr;

	// 입력 복구
	if (bInputDisabledDuringCutscene)
	{
		EnablePlayerInput();
	}

	bIsPlayingCutscene = false;
	bInputDisabledDuringCutscene = false;

	OnCutsceneFinished.Broadcast();
}

void UERNCutsceneSubsystem::OnCutsceneComplete()
{
	StopCutscene();
}

void UERNCutsceneSubsystem::DisablePlayerInput()
{
	UWorld* World = GetGameInstance()->GetWorld();
	if (!World)
	{
		return;
	}

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		if (APlayerController* PC = It->Get())
		{
			PC->SetIgnoreMoveInput(true);
			PC->SetIgnoreLookInput(true);
		}
	}
}

void UERNCutsceneSubsystem::EnablePlayerInput()
{
	UWorld* World = GetGameInstance()->GetWorld();
	if (!World)
	{
		return;
	}

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		if (APlayerController* PC = It->Get())
		{
			PC->ResetIgnoreMoveInput();
			PC->ResetIgnoreLookInput();
		}
	}
}
