// Copyright Epic Games, Inc. All Rights Reserved.

#include "Subsystem/ERNCutsceneSubsystem.h"
#include "Blueprint/UserWidget.h"
#include "Engine/GameViewportClient.h"
#include "Kismet/GameplayStatics.h"
#include "LevelSequence.h"
#include "LevelSequencePlayer.h"
#include "LevelSequenceActor.h"
#include "MovieSceneSequencePlayer.h"
#include "MovieScene.h"
#include "Core/ERNGameInstance.h"
#include "Widgets/SWeakWidget.h"
#include "Character/ERNCharacterBase.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/GameStateBase.h"
#include "EngineUtils.h"
#include "Inventory/Components/ERNEquipmentComponent.h"
#include "Actors/Intro/ERNIntroBird.h"
#include "Character/Player/ProjectERNCharacter.h"
#include "Character/Player/ERNPlayerController.h"
#include "Components/SceneComponent.h"
#include "GameFramework/GameModeBase.h"
#include "Subsystem/ERNSoundSubsystem.h"
#include "Inventory/Item/Manager/ItemManagerSubsystem.h"
#include "Inventory/Item/Data/ERNItemRuntimeState.h"
#include "Camera/PlayerCameraManager.h"
#include "MovieSceneObjectBindingID.h"
#include "TimerManager.h"

void UERNCutsceneSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// 맵 로딩 완료 시 로딩 화면 숨김 처리
	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UERNCutsceneSubsystem::OnPostLoadMapWithWorld);
}

void UERNCutsceneSubsystem::Deinitialize()
{
	// 델리게이트 해제
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

void UERNCutsceneSubsystem::OnPostLoadMapWithWorld(UWorld* LoadedWorld)
{
	if (!LoadedWorld)
	{
		HideLoadingScreen();
		return;
	}

	// SeamlessTravel 도중 슬레이트가 무효화되거나 분리될 수 있음 — 로딩 중이면 옛 위젯을 즉시 분리하고
	// 재부착은 다음 틱으로 지연 (클라에서 SeamlessTravel 직후 LocalPlayer/HUD 재구성으로 viewport overlay 상태가
	// 어긋난 프레임에 추가하면 렌더 패스가 누락되는 케이스 방지)
	if (bIsLoading)
	{
		if (LoadingSlateWidget.IsValid())
		{
			if (UGameViewportClient* VC = GetGameInstance() ? GetGameInstance()->GetGameViewportClient() : nullptr)
			{
				VC->RemoveViewportWidgetContent(LoadingSlateWidget.ToSharedRef());
			}
			LoadingSlateWidget.Reset();
		}
		LoadingWidget = nullptr;

		LoadedWorld->GetTimerManager().SetTimerForNextTick(
			FTimerDelegate::CreateUObject(this, &UERNCutsceneSubsystem::EnsureLoadingWidgetInViewport));
	}

	// 필드맵 진입 시 아이템 메시/PSO 웜업 (카메라/폰 준비 후 시야 앞에서 미리 렌더 → 첫 드롭 히칭 완화)
	WarmUpRetryCount = 0;
	WarmUpFieldItems();

	// 9초 시점에 새 인트로 시작 (서버에서 spawn + 부착) — 1초 동안 클라 리플리케이션 도착할 시간 확보
	if (UERNGameInstance* GameInst = Cast<UERNGameInstance>(GetGameInstance()))
	{
		if (GameInst->ShouldAutoStartIntro())
		{
			FTimerHandle IntroTimerHandle;
			LoadedWorld->GetTimerManager().SetTimer(
				IntroTimerHandle,
				this,
				&UERNCutsceneSubsystem::StartBirdIntroSequence,
				9.0f,
				false
			);
		}
	}

	// 로딩 화면 제거 — 로비맵은 인트로가 없으므로 5초, 그 외(필드/보스, 인트로 연출)는 10초
	const bool bIsLobbyMap = LoadedWorld->GetMapName().Contains(TEXT("Map_Lobby"));
	const float HideLoadingDelay = bIsLobbyMap ? 5.0f : 10.0f;
	FTimerHandle TimerHandle;
	LoadedWorld->GetTimerManager().SetTimer(
		TimerHandle,
		this,
		&UERNCutsceneSubsystem::HideLoadingScreen,
		HideLoadingDelay,
		false
	);
}

void UERNCutsceneSubsystem::EnsureLoadingWidgetInViewport()
{
	// 이미 살아있는 슬레이트가 부착돼 있으면 no-op
	if (LoadingSlateWidget.IsValid())
	{
		return;
	}

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
		// 낮은 레벨의 Slate 위젯으로 추가 (맵 전환에도 유지됨 — 클라는 OnPostLoadMapWithWorld에서 재부착)
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

	EnsureLoadingWidgetInViewport();
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

	// 새 인트로는 로딩 종료 1초 전(OnPostLoadMapWithWorld의 9초 타이머)에 이미 시작됨 — 여기선 호출 안 함
}

// ===== 에셋 웜업 =====

void UERNCutsceneSubsystem::WarmUpFieldItems()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// 데디케이티드 서버는 렌더링이 없으므로 웜업 불필요
	if (World->GetNetMode() == ENetMode::NM_DedicatedServer)
	{
		return;
	}

	UItemManagerSubsystem* ItemManager = GetGameInstance() ? GetGameInstance()->GetSubsystem<UItemManagerSubsystem>() : nullptr;
	if (!ItemManager)
	{
		return;
	}

	// 카메라/폰 준비 확인 — PostLoadMap 직후엔 아직 미possess이거나 카메라가 원점(0,0,0)이라
	// 여기서 스폰하면 시야 밖이라 컬링되어 PSO가 안 구워진다. 준비될 때까지 짧게 재시도
	APlayerController* PC = World->GetFirstPlayerController();
	const APawn* Pawn = PC ? PC->GetPawn() : nullptr;
	const bool bCameraReady = PC && PC->PlayerCameraManager &&
		!PC->PlayerCameraManager->GetCameraLocation().IsNearlyZero();
	if (!PC || !Pawn || !bCameraReady)
	{
		if (WarmUpRetryCount < WarmUpMaxRetries)
		{
			++WarmUpRetryCount;
			FTimerHandle RetryTimerHandle;
			World->GetTimerManager().SetTimer(
				RetryTimerHandle,
				this,
				&UERNCutsceneSubsystem::WarmUpFieldItems,
				0.1f,
				false);
		}
		return;
	}

	const FVector CamLoc = PC->PlayerCameraManager->GetCameraLocation();
	const FVector CamForward = PC->PlayerCameraManager->GetCameraRotation().Vector();
	// 카메라 코앞
	const FVector WarmUpLocation = CamLoc + CamForward * 300.f;

	// ItemTable의 모든 아이템을 그 위치에 한 번씩 spawn → 메시 적용 시 PSO 컴파일 트리거
	const TArray<FName> AllItemIDs = ItemManager->GetAllItemIDs();
	for (const FName& ItemID : AllItemIDs)
	{
		if (!ItemManager->ItemValid(ItemID))
		{
			continue;
		}

		FItemRuntimeState RuntimeState;
		RuntimeState.SetItemID(ItemID);
		RuntimeState.SetQuantity(1);
		AActor* WarmUpItem = ItemManager->SpawnItem(RuntimeState, WarmUpLocation, FRotator::ZeroRotator);
		if (WarmUpItem)
		{
			// 순수 로컬 렌더 웜업용 — 리슨 호스트에서 클라로 복제되지 않도록 차단
			WarmUpItem->SetReplicates(false);
			// 습득/충돌 방지
			WarmUpItem->SetActorEnableCollision(false);
			WarmUpActors.Add(WarmUpItem);
		}
	}

	if (WarmUpActors.Num() == 0)
	{
		return;
	}

	// 몇 프레임 렌더링되도록 잠시 살려둔 뒤 일괄 제거 (즉시 destroy하면 한 번도 안 그려져 PSO 미생성)
	FTimerHandle CleanupTimerHandle;
	World->GetTimerManager().SetTimer(
		CleanupTimerHandle,
		this,
		&UERNCutsceneSubsystem::CleanupWarmUpActors,
		2.0f,
		false);
}

void UERNCutsceneSubsystem::CleanupWarmUpActors()
{
	for (AActor* WarmUpItem : WarmUpActors)
	{
		if (IsValid(WarmUpItem))
		{
			WarmUpItem->Destroy();
		}
	}
	WarmUpActors.Empty();
}

// ===== 인트로 시퀀스 =====

void UERNCutsceneSubsystem::StartBirdIntroSequence()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// 서버 권한 체크 (Authority GameMode가 있어야 서버)
	if (World->GetAuthGameMode() == nullptr)
	{
		UE_LOG(LogTemp, Log, TEXT("[IntroSequence] Skip — client side"));
		return;
	}

	UERNGameInstance* GameInst = Cast<UERNGameInstance>(GetGameInstance());
	if (!GameInst)
	{
		UE_LOG(LogTemp, Warning, TEXT("[IntroSequence] GameInstance cast failed"));
		return;
	}

	const TSubclassOf<AERNIntroBird> IntroBirdClass = GameInst->GetIntroBirdClass();
	if (!IntroBirdClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[IntroSequence] IntroBirdClass not set on BP_ERNGameInstance"));
		return;
	}

	const float FadeInDuration = GameInst->GetIntroFadeInDuration();

	// 맵의 모든 IntroSpawnGroup 액터 수집 (태그 "IntroSpawnGroup")
	TArray<AActor*> SpawnGroups;
	UGameplayStatics::GetAllActorsWithTag(World, FName(TEXT("IntroSpawnGroup")), SpawnGroups);

	if (SpawnGroups.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[IntroSequence] No IntroSpawnGroup found in map"));
		return;
	}

	// 그룹 1개 랜덤 선택
	AActor* ChosenGroup = SpawnGroups[FMath::RandRange(0, SpawnGroups.Num() - 1)];

	// 그룹의 자식 SceneComponent들을 슬롯으로 수집 (Root 제외)
	TArray<USceneComponent*> Slots;
	if (USceneComponent* RootComp = ChosenGroup->GetRootComponent())
	{
		TArray<USceneComponent*> Children;
		RootComp->GetChildrenComponents(true, Children);
		for (USceneComponent* Child : Children)
		{
			// 같은 액터 소속 + 이름이 "Slot_"으로 시작하는 컴포넌트만
			if (Child && Child->GetOwner() == ChosenGroup && Child->GetName().StartsWith(TEXT("Slot_")))
			{
				Slots.Add(Child);
			}
		}
	}

	if (Slots.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[IntroSequence] Chosen group has no Slot_ components"));
		return;
	}

	// 슬롯 셔플 (Fisher-Yates)
	for (int32 i = Slots.Num() - 1; i > 0; --i)
	{
		const int32 j = FMath::RandRange(0, i);
		Slots.Swap(i, j);
	}

	// 모든 PlayerController 수집
	TArray<AERNPlayerController*> PCs;
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		if (AERNPlayerController* PC = Cast<AERNPlayerController>(It->Get()))
		{
			PCs.Add(PC);
		}
	}

	// min(PCs, Slots) 만큼 매칭 — 새 스폰 + 부착 + 페이드 인
	const int32 Count = FMath::Min(PCs.Num(), Slots.Num());
	for (int32 i = 0; i < Count; ++i)
	{
		AERNPlayerController* PC = PCs[i];
		USceneComponent* Slot = Slots[i];
		if (!PC || !Slot) continue;

		AProjectERNCharacter* Char = Cast<AProjectERNCharacter>(PC->GetPawn());
		if (!Char) continue;

		// 새 스폰
		const FTransform SpawnXform = Slot->GetComponentTransform();
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		AERNIntroBird* Bird = World->SpawnActor<AERNIntroBird>(IntroBirdClass, SpawnXform, SpawnParams);
		if (!Bird) continue;

		// 캐릭터 부착 + 매달림 상태 켜기 + 몽타주 재생
		Char->AttachToIntroBird(Bird);

		// 새 비행 시작
		Bird->StartFlight();

		// 페이드 인 (해당 클라에)
		PC->Client_StartFadeIn(FadeInDuration);

		// 인트로 타이틀 위젯 표시 (위젯 자체 애니메이션으로 페이드 인/홀드/페이드 아웃)
		PC->Client_ShowIntroTitleWidget();
	}

	UE_LOG(LogTemp, Log, TEXT("[IntroSequence] Started with %d birds (group: %s)"), Count, *ChosenGroup->GetName());
}

// ===== 컷신 시스템 =====

void UERNCutsceneSubsystem::PlayCutscene(ULevelSequence* Sequence, bool bDisablePlayerInput)
{
	PlayCutsceneInternal(Sequence, bDisablePlayerInput, true);
}

void UERNCutsceneSubsystem::PlayCutsceneWithoutPlayers(ULevelSequence* Sequence, bool bDisablePlayerInput)
{
	PlayCutsceneInternal(Sequence, bDisablePlayerInput, false);
}

void UERNCutsceneSubsystem::PlayCutsceneInternal(ULevelSequence* Sequence, bool bDisablePlayerInput, bool bBindPlayers)
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

	// 컷신 진입 시 BGM 페이드아웃 정지
	if (UERNSoundSubsystem* SoundSubsystem = GetGameInstance()->GetSubsystem<UERNSoundSubsystem>())
	{
		SoundSubsystem->StopBGM(1.f);
	}

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
			// 플레이어 바인딩
			if (bBindPlayers)
			{
				BindPlayersToSequence(CurrentSequencePlayer, Sequence);
			}

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

	// ViewTarget을 각 PC의 Pawn으로 명시적 복원
	// — 멀티플레이에서 클라이언트의 ViewTarget이 시퀀서 카메라에서 자동 복귀 안 되는 문제 방지
	// — 각 머신이 자기 World의 PC만 순회 (서버는 호스트 PC, 클라는 자기 PC)
	if (UWorld* World = GetWorld())
	{
		for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
		{
			if (APlayerController* PC = It->Get())
			{
				if (APawn* Pawn = PC->GetPawn())
				{
					PC->SetViewTargetWithBlend(Pawn, 0.f);
				}
			}
		}
	}

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
			if (APawn* Pawn = PC->GetPawn())
			{
				Pawn->DisableInput(PC);
			}
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
			if (APawn* Pawn = PC->GetPawn())
			{
				Pawn->EnableInput(PC);
			}
		}
	}
}

void UERNCutsceneSubsystem::BindPlayersToSequence(ULevelSequencePlayer* Player, ULevelSequence* Sequence)
{
	if (!Player || !Sequence)
	{
		return;
	}

	UWorld* World = GetGameInstance()->GetWorld();
	if (!World)
	{
		return;
	}

	UMovieScene* MovieScene = Sequence->GetMovieScene();
	if (!MovieScene)
	{
		return;
	}

	// 모든 플레이어 캐릭터 수집 (GameState->PlayerArray 사용 - 클라이언트에서도 모든 플레이어 접근 가능)
	TArray<AERNCharacterBase*> PlayerCharacters;
	if (AGameStateBase* GameState = World->GetGameState())
	{
		for (APlayerState* PS : GameState->PlayerArray)
		{
			if (PS)
			{
				if (AERNCharacterBase* Character = Cast<AERNCharacterBase>(PS->GetPawn()))
				{
					PlayerCharacters.Add(Character);
				}
			}
		}
	}

	const int32 PlayerCount = PlayerCharacters.Num();

	// 시퀀서의 Possessable 바인딩 순회
	for (int32 i = 0; i < MovieScene->GetPossessableCount(); i++)
	{
		const FMovieScenePossessable& Possessable = MovieScene->GetPossessable(i);
		FString PossessableName = Possessable.GetName();

		// "Player1", "Player2", "Player3" 이름의 Possessable 처리
		for (int32 PlayerIndex = 0; PlayerIndex < 3; PlayerIndex++)
		{
			FString ExpectedName = FString::Printf(TEXT("Player%d"), PlayerIndex + 1);
			if (PossessableName.Equals(ExpectedName, ESearchCase::IgnoreCase))
			{
				// 해당 인덱스의 플레이어가 존재하면 바인딩
				if (PlayerIndex < PlayerCount)
				{
					// 기존 코드. 시퀀스에 영구적으로 추가됨.
					//Sequence->BindPossessableObject(Possessable.GetGuid(), *PlayerCharacters[PlayerIndex], World);
					
					// 시퀀스에 영구적으로 추가하지 않고 일회성으로 추가
					if (CurrentSequenceActor)
					{
						TArray<AActor*> BoundActors;
						BoundActors.Add(PlayerCharacters[PlayerIndex]);

						const FMovieSceneObjectBindingID BindingID(
							UE::MovieScene::FRelativeObjectBindingID(Possessable.GetGuid())
						);

						CurrentSequenceActor->SetBinding(BindingID, BoundActors, false);
					}
					
					
					UE_LOG(LogTemp, Log, TEXT("[CutsceneSubsystem] Bound %s to %s"),
						*ExpectedName, *PlayerCharacters[PlayerIndex]->GetName());
				}

				// 플레이스홀더 액터 Destroy (바인딩 여부와 관계없이 항상 제거)
				for (TActorIterator<AActor> It(World); It; ++It)
				{
					AActor* Actor = *It;
					if (Actor && Actor->GetActorNameOrLabel() == ExpectedName)
					{
						// 장착된 무기 먼저 Destroy
						if (UERNEquipmentComponent* EquipComp = Actor->FindComponentByClass<UERNEquipmentComponent>())
						{
							if (EquipComp->CurrentWeapon)
							{
								EquipComp->CurrentWeapon->Destroy();
							}
						}

						UE_LOG(LogTemp, Log, TEXT("[CutsceneSubsystem] Destroying placeholder for %s: %s"),
							*ExpectedName, *Actor->GetName());
						Actor->Destroy();
						break;
					}
				}
				break;
			}
		}
	}
}
