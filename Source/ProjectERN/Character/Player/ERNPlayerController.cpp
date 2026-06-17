// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/Player/ERNPlayerController.h"

#include "EngineUtils.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "OnlineSubsystemUtils.h"
#include "Blueprint/UserWidget.h"
#include "ProjectERN.h"
#include "ProjectERNCharacter.h"
#include "Actors/Church.h"
#include "Actors/DHRollActor.h"
#include "Widgets/Input/SVirtualJoystick.h"
#include "Character/Player/ERNPlayerState.h"
#include "Core/ERNGameInstance.h"
#include "Core/ERNGameState.h"
#include "Interfaces/IInteractable.h"
#include "UI/ERNInventoryWidget.h"
#include "UI/ERNEntranceWidget.h"
#include "UI/ERNUIManagerSubsystem.h"
#include "UI/ERNDamageTextActor.h"
#include "UI/ERNBossHealthBarWidget.h"
#include "Character/Enemy/ERNBossCharacter.h"
#include "Camera/CameraShakeBase.h"
#include "Components/PostProcessComponent.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UI/World/ERNMinimapWidget.h"
#include "UI/World/ERNCompassWidget.h"
#include "World/ERNMinimapPinPoint.h"
#include "Subsystem/ERNCutsceneSubsystem.h"
#include "UI/ERNDownedStatusWidget.h"
#include "UI/ERNSkillCoolPanel.h"
#include "WorldPartition/WorldPartitionSubsystem.h"
#include "UI/ERNQuickSlotWidget.h"
#include "UI/ERNInteractableWidget.h"

#include "UI/ERNPlayerDetailStatusWidget.h"

void AERNPlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	// 새 레벨(컨트롤러) 시작 시 UI 상태 초기화
	if (const ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (UERNUIManagerSubsystem* UIManager = LocalPlayer->GetSubsystem<UERNUIManagerSubsystem>())
		{
			UIManager->ResetUIState();
		}
	}

	// 컷신 동안 HUD 일괄 숨김 (로컬 컨트롤러 전용)
	BindCutsceneEvents();

	// only spawn touch controls on local player controllers
	if (ShouldUseTouchControls() && IsLocalPlayerController())
	{
		// spawn the mobile controls widget
		MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);

		if (MobileControlsWidget)
		{
			// add the controls to the player screen
			MobileControlsWidget->AddToPlayerScreen(0);
			RegisterHUDWidget(MobileControlsWidget);
		}
		else
		{
			UE_LOG(LogProjectERN, Error, TEXT("Could not spawn mobile controls widget."));
		}
	}

	// 현재 맵 이름 가져오기 (UI 생성용)
	FString CurrentMapName = GetWorld()->GetMapName();

	// 명시적으로 게임 모드로 초기화하여 이전 맵(메인 메뉴 등)에서 넘어온 UI 포커스를 해제
	// 단, 메인 메뉴(MainMenu) 맵인 경우에는 예외 (UI 조작이 필요하므로)
	if (IsLocalPlayerController() && !CurrentMapName.Contains(TEXT("MainMenu")))
	{
		FInputModeGameOnly InputMode;
		SetInputMode(InputMode);
		bShowMouseCursor = false;
	}

	// 파티 상태 위젯 생성 (로컬 플레이어만)
	if (IsLocalPlayerController() && PartyStatusContainerClass)
	{
		// 숨겨야 할 맵인지 확인
		bool bShouldHide = false;
		for (const FString& MapName : HidePartyWidgetMapNames)
		{
			if (CurrentMapName.Contains(MapName))
			{
				bShouldHide = true;
				break;
			}
		}

		// 숨겨야 할 맵이 아니면 위젯 생성
		if (!bShouldHide)
		{
			UUserWidget* Container = CreateWidget<UUserWidget>(this, PartyStatusContainerClass);
			if (Container)
			{
				Container->AddToViewport();
				RegisterHUDWidget(Container);
			}
		}
	}

	// 준비 완료 버튼 위젯 생성 (로컬 플레이어만)
	if (IsLocalPlayerController() && ReadyButtonWidgetClass)
	{
		// 숨겨야 할 맵인지 확인
		bool bShouldHide = false;
		for (const FString& MapName : HideReadyButtonMapNames)
		{
			if (CurrentMapName.Contains(MapName))
			{
				bShouldHide = true;
				break;
			}
		}

		// 숨겨야 할 맵이 아니면 위젯 생성
		if (!bShouldHide)
		{
			UUserWidget* ReadyButton = CreateWidget<UUserWidget>(this, ReadyButtonWidgetClass);
			if (ReadyButton)
			{
				ReadyButton->AddToViewport();
				RegisterHUDWidget(ReadyButton);
			}
		}
	}

	// 인벤토리 위젯 생성 (로컬 플레이어만)
	if (IsLocalPlayerController() && InventoryWidgetClass)
	{
		// 숨겨야 할 맵인지 확인
		bool bShouldHide = false;
		for (const FString& MapName : HideInventoryWidgetMapNames)
		{
			if (CurrentMapName.Contains(MapName))
			{
				bShouldHide = true;
				break;
			}
		}

		// 숨겨야 할 맵이 아니면 위젯 생성
		if (!bShouldHide)
		{
			InventoryWidget = CreateWidget<UUserWidget>(this, InventoryWidgetClass);
			if (InventoryWidget)
			{
				if (UERNInteractableWidget* InteractableInventoryWidget = Cast<UERNInteractableWidget>(InventoryWidget))
				{
					InteractableInventoryWidget->OnWidgetClosed.AddUniqueDynamic(this, &AERNPlayerController::InventoryClose);
				}
				InventoryWidget->AddToViewport(100);
				RegisterHUDWidget(InventoryWidget);
				RefreshInventoryWidget();
			}
		}
	}

	// 상태정보창 위젯 생성 (로컬 플레이어만)
	if (IsLocalPlayerController() && StatusWidgetClass)
	{
		bool bShouldHide = false;
		for (const FString& MapName : HideStatusWidgetMapNames)
		{
			if (CurrentMapName.Contains(MapName))
			{
				bShouldHide = true;
				break;
			}
		}

		if (!bShouldHide)
		{
			StatusWidget = CreateWidget<UERNPlayerDetailStatusWidget>(this, StatusWidgetClass);
			if (StatusWidget)
			{
				StatusWidget->AddToViewport(150);
				StatusWidget->SetVisibility(ESlateVisibility::Hidden);
				RegisterHUDWidget(StatusWidget);
			}
		}
	}

	// 미니맵 위젯 생성 (로컬 플레이어만)
	if (IsLocalPlayerController() && MinimapWidgetClass)
	{
		bool bShouldHide = false;
		for (const FString& MapName : HideMinimapWidgetMapNames)
		{
			if (CurrentMapName.Contains(MapName))
			{
				bShouldHide = true;
				break;
			}
		}

		if (!bShouldHide)
		{
			MinimapWidget = CreateWidget<UUserWidget>(this, MinimapWidgetClass);
			if (MinimapWidget)
			{
				if (UERNInteractableWidget* InteractableMinimapWidget = Cast<UERNInteractableWidget>(MinimapWidget))
				{
					InteractableMinimapWidget->OnWidgetClosed.AddUniqueDynamic(
						this, &AERNPlayerController::MinimapClose);
				}
				MinimapWidget->AddToViewport(1000);
				RegisterHUDWidget(MinimapWidget);
			}
		}
	}

	// 나침반 위젯 생성 (로컬 플레이어만)
	if (IsLocalPlayerController() && CompassWidgetClass)
	{
		bool bShouldHide = false;
		for (const FString& MapName : HideCompassWidgetMapNames)
		{
			if (CurrentMapName.Contains(MapName))
			{
				bShouldHide = true;
				break;
			}
		}

		if (!bShouldHide)
		{
			CompassWidget = CreateWidget<UUserWidget>(this, CompassWidgetClass);
			if (CompassWidget)
			{
				CompassWidget->AddToViewport(50);
				RegisterHUDWidget(CompassWidget);
			}
		}
	}

	// 채팅 위젯 생성 (로컬 플레이어만)
	if (IsLocalPlayerController() && ChatWidgetClass)
	{
		// 숨겨야 할 맵인지 확인 (메인 메뉴 등)
		bool bShouldHide = false;
		for (const FString& MapName : HideChatWidgetMapNames)
		{
			if (CurrentMapName.Contains(MapName))
			{
				bShouldHide = true;
				break;
			}
		}

		if (!bShouldHide)
		{
			ChatWidget = CreateWidget<UUserWidget>(this, ChatWidgetClass);
			if (ChatWidget)
			{
				ChatWidget->AddToViewport(50);	// ZOrder: HUD 위, 메뉴 아래
				RegisterHUDWidget(ChatWidget);
			}
		}
	}
	
	// 스킬 쿨타임 패널 생성 (로컬 플레이어만)
	if (IsLocalPlayerController() && SkillCoolPanelWidgetClass)
	{
		// 숨겨야 할 맵인지 확인 (메인 메뉴 등)
		bool bShouldHide = false;
		for (const FString& MapName : HideSkillCoolPanelMapNames)
		{
			if (CurrentMapName.Contains(MapName))
			{
				bShouldHide = true;
				break;
			}
		}

		if (!bShouldHide)
		{
			SkillCoolPanelWidget = CreateWidget<UERNSkillCoolPanel>(this, SkillCoolPanelWidgetClass);

			if (SkillCoolPanelWidget)
			{
				SkillCoolPanelWidget->AddToViewport(50);
				// 컷신 시작 시 함께 숨겨지도록 등록한다.
				RegisterHUDWidget(SkillCoolPanelWidget);
			}
		}
	}

	// 퀵슬롯 패널 생성 (로컬 플레이어만)
	if (IsLocalPlayerController() && QuickSlotWidgetClass)
	{
		// 숨겨야 할 맵인지 확인 (메인 메뉴 등)
		bool bShouldHide = false;
		for (const FString& MapName : HideQuickSlotMapNames)
		{
			if (CurrentMapName.Contains(MapName))
			{
				bShouldHide = true;
				break;
			}
		}

		if (!bShouldHide)
		{
			QuickSlotWidget = CreateWidget<UERNQuickSlotWidget>(this, QuickSlotWidgetClass);

			if (QuickSlotWidget)
			{
				QuickSlotWidget->AddToViewport(50);
				// 컷신 시작 시 함께 숨겨지도록 등록한다.
				RegisterHUDWidget(QuickSlotWidget);
			}
		}
	}
	
	// 기절 상태 UI 생성
	InitializeSelfDownedStatusWidget();
	RefreshSelfDownedStatusWidget();
	
	// 닉네임 전송 (로컬 플레이어만) - 타이머로 재시도
	if (IsLocalPlayerController())
	{
		// 0.1초 후에 시도 (PlayerState가 준비될 시간 확보)
		FTimerHandle TimerHandle;
		GetWorld()->GetTimerManager().SetTimer(TimerHandle, this, &AERNPlayerController::TrySendNickname, 0.1f, false);

		// 캐릭터 타입 체크 시작 시간 기록
		CharacterTypeCheckStartTime = GetWorld()->GetTimeSeconds();

		// 0.3초마다 반복 체크 (최대 10초 동안)
		GetWorld()->GetTimerManager().SetTimer(CharacterTypeCheckTimerHandle, this, &AERNPlayerController::CheckAndFixCharacterType, 0.3f, true);
	}
	
	// TODO : 검증 필요
	// 로비 맵 진입 시 준비 상태 초기화
	if (CurrentMapName.Contains(TEXT("Lobby")))
	{
		if (AERNPlayerState* PS = GetPlayerState<AERNPlayerState>())
		{
			if (PS->bIsReady)
			{
				PS->Server_SetReady(false);
				UE_LOG(LogTemp, Log, TEXT("Reset ready state on lobby entry"));
			}
		}
	}
}

void AERNPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (SelfDownedStatusWidget)
	{
		UnregisterHUDWidget(SelfDownedStatusWidget);
		SelfDownedStatusWidget->SetObservedCharacter(nullptr);
		SelfDownedStatusWidget->RemoveFromParent();
		SelfDownedStatusWidget = nullptr;
	}
	
	UnbindCutsceneEvents();
	Super::EndPlay(EndPlayReason);
}

void AERNPlayerController::RegisterHUDWidget(UUserWidget* Widget)
{
	if (Widget)
	{
		ManagedHUDWidgets.AddUnique(Widget);

		// 등록하는 시점에 이미 컷신이 재생 중이라면 즉시 숨김 처리 (호스트 등 타이밍 이슈 방어)
		if (UGameInstance* GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr)
		{
			if (UERNCutsceneSubsystem* Cutscene = GI->GetSubsystem<UERNCutsceneSubsystem>())
			{
				if (Cutscene->IsPlayingCutscene())
				{
					CachedHUDVisibilities.Add(Widget, Widget->GetVisibility());
					Widget->SetVisibility(ESlateVisibility::Collapsed);
				}
			}
		}
	}
}

void AERNPlayerController::UnregisterHUDWidget(UUserWidget* Widget)
{
	if (Widget)
	{
		ManagedHUDWidgets.Remove(Widget);
		CachedHUDVisibilities.Remove(Widget);
	}
}

void AERNPlayerController::BindCutsceneEvents()
{
	if (!IsLocalPlayerController())
	{
		return;
	}

	UGameInstance* GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	if (GI == nullptr)
	{
		return;
	}

	if (UERNCutsceneSubsystem* Cutscene = GI->GetSubsystem<UERNCutsceneSubsystem>())
	{
		Cutscene->OnCutsceneStarted.AddDynamic(this, &AERNPlayerController::HandleCutsceneStarted);
		Cutscene->OnCutsceneFinished.AddDynamic(this, &AERNPlayerController::HandleCutsceneFinished);
	}
}

void AERNPlayerController::UnbindCutsceneEvents()
{
	UGameInstance* GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
	if (GI == nullptr)
	{
		return;
	}

	if (UERNCutsceneSubsystem* Cutscene = GI->GetSubsystem<UERNCutsceneSubsystem>())
	{
		Cutscene->OnCutsceneStarted.RemoveDynamic(this, &AERNPlayerController::HandleCutsceneStarted);
		Cutscene->OnCutsceneFinished.RemoveDynamic(this, &AERNPlayerController::HandleCutsceneFinished);
	}
}

void AERNPlayerController::HandleCutsceneStarted()
{
	// 컷신 진입 시 열린 인터랙티브 UI를 모두 닫아 포커스/입력 모드를 게임으로 회수
	// (안 닫으면 컷신 종료 후 InputMode가 GameAndUI/UIOnly로 남아 입력이 안 먹음)
	InventoryClose();
	MinimapClose();
	ClosePauseMenu();

	UERNUIManagerSubsystem* UIManager = ULocalPlayer::GetSubsystem<UERNUIManagerSubsystem>(GetLocalPlayer());
	if (UIManager)
	{
		UIManager->CloseActiveUI();
	}

	// 채팅 등 BP가 포커스를 쥐고 있는 UI를 닫도록 알림
	OnCutsceneForceCloseUI();

	// 남아있는 키보드 포커스까지 게임 뷰포트로 회수
	SetInputMode(FInputModeGameOnly());
	SetShowMouseCursor(false);

	CachedHUDVisibilities.Reset();

	for (const TObjectPtr<UUserWidget>& Widget : ManagedHUDWidgets)
	{
		if (IsValid(Widget))
		{
			CachedHUDVisibilities.Add(Widget, Widget->GetVisibility());
			Widget->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void AERNPlayerController::HandleCutsceneFinished()
{
	for (const TObjectPtr<UUserWidget>& Widget : ManagedHUDWidgets)
	{
		if (IsValid(Widget) == false)
		{
			continue;
		}

		// 컷신 시작 시 캐싱한 visibility로 복원 (없으면 기본 표시)
		const ESlateVisibility* CachedVisibility = CachedHUDVisibilities.Find(Widget);
		Widget->SetVisibility(CachedVisibility ? *CachedVisibility : ESlateVisibility::SelfHitTestInvisible);
	}

	CachedHUDVisibilities.Reset();

	// 컷신 종료 후 게임플레이 입력이 확실히 먹도록 입력 모드를 게임으로 고정
	SetInputMode(FInputModeGameOnly());
	SetShowMouseCursor(false);

	// 컷신 동안 'UI 열림' 상태로 캐싱되어 숨겨진 채 복원된 HUD(스킬/퀵슬롯/골드 등 OnUIStateChanged 구독)와
	// TryInteract 게이트를 정상화 — 인벤토리를 I로 닫았을 때와 동일하게 UI 상태 None을 재브로드캐스트한다.
	// (HUD 복원 루프 뒤에 실행되어 캐시 타이밍과 무관하게 최종 보정)
	UERNUIManagerSubsystem* UIManager = ULocalPlayer::GetSubsystem<UERNUIManagerSubsystem>(GetLocalPlayer());
	if (UIManager)
	{
		UIManager->ForceCloseAllUI();
	}
}

void AERNPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	RefreshSelfDownedStatusWidget();
}

void AERNPlayerController::AcknowledgePossession(class APawn* P)
{
	Super::AcknowledgePossession(P);

	RefreshInventoryWidget();
	RefreshSkillCoolPanel();
	RefreshQuickSlotWidget();
	RefreshSelfDownedStatusWidget();
}

void AERNPlayerController::RefreshInventoryWidget() const
{
	if (!IsLocalPlayerController())
	{
		return;
	}
	
	if (UERNInventoryWidget* ERNInventoryWidget = Cast<UERNInventoryWidget>(InventoryWidget))
	{
		ERNInventoryWidget->RefreshFromCurrentCharacter();
	}
}

void AERNPlayerController::RefreshQuickSlotWidget() const
{
	if (!IsLocalPlayerController())
	{
		return;
	}
	
	if (QuickSlotWidget)
	{
		QuickSlotWidget->RefreshFromCurrentCharacter();
	}
}

void AERNPlayerController::TrySendNickname()
{
	if (!IsLocalPlayerController())
	{
		return;
	}

	UERNGameInstance* GI = Cast<UERNGameInstance>(GetGameInstance());
	AERNPlayerState* PS = GetPlayerState<AERNPlayerState>();

	UE_LOG(LogTemp, Warning, TEXT("[DEBUG] GI valid: %s, PS valid: %s, Nickname: %s, CharacterType: %d"),
		GI ? TEXT("YES") : TEXT("NO"),
		PS ? TEXT("YES") : TEXT("NO"),
		GI ? *GI->CurrentPlayerNickname : TEXT("GI is null"),
		GI ? static_cast<int32>(GI->GetPlayerCharacterType()) : -1);

	if (GI && PS)
	{
		// 닉네임 전송
		if (!GI->CurrentPlayerNickname.IsEmpty())
		{
			PS->Server_SetNickname(GI->CurrentPlayerNickname);
			UE_LOG(LogTemp, Log, TEXT("Sending nickname to server: %s"), *GI->CurrentPlayerNickname);
		}

		// CharacterType 복원 (None이 아닌 경우에만)
		if (GI->GetPlayerCharacterType() != ECharacterType::None)
		{
			PS->Server_SetCharacterType(GI->GetPlayerCharacterType());
			UE_LOG(LogTemp, Log, TEXT("Requesting CharacterType restoration to server: %d"), static_cast<int32>(GI->GetPlayerCharacterType()));
		}
	}
	else if (GI && !GI->CurrentPlayerNickname.IsEmpty())
	{
		// PlayerState가 아직 null이면 0.1초 후 재시도 (최대 50번)
		static int32 RetryCount = 0;
		if (RetryCount < 50)
		{
			RetryCount++;
			UE_LOG(LogTemp, Warning, TEXT("PlayerState not ready, retrying... (Attempt %d/50)"), RetryCount);
			FTimerHandle RetryTimer;
			GetWorld()->GetTimerManager().SetTimer(RetryTimer, this, &AERNPlayerController::TrySendNickname, 0.1f, false);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to send nickname: PlayerState never became valid"));
			RetryCount = 0;
		}
	}
}

void AERNPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// 이 시점에서는 LocalPlayer가 확실히 할당되어 있으므로 IsLocalPlayerController()가 true를 반환합니다.
	RefreshSelfDownedStatusWidget();
	
	// only add IMCs for local player controllers
	if (IsLocalPlayerController())
	{
		// Add Input Mapping Contexts
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
			{
				Subsystem->AddMappingContext(CurrentContext, 0);
			}

			// only add these IMCs if we're not using mobile touch input
			if (!ShouldUseTouchControls())
			{
				for (UInputMappingContext* CurrentContext : MobileExcludedMappingContexts)
				{
					Subsystem->AddMappingContext(CurrentContext, 0);
				}
			}
		}

		// Bind Input Actions
		if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
		{
			if (ReadyToggleAction)
			{
				EnhancedInputComponent->BindAction(ReadyToggleAction, ETriggerEvent::Triggered, this, &AERNPlayerController::ToggleReady);
			}

			if (InteractAction)
			{
				EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, this, &AERNPlayerController::TryInteract);
			}
			
			if (InventoryAction)
			{
				EnhancedInputComponent->BindAction(InventoryAction, ETriggerEvent::Started, this,
				                                   &AERNPlayerController::InventoryOpen);
			}
			
			if (StatusAction)
			{
				EnhancedInputComponent->BindAction(StatusAction, ETriggerEvent::Started, this, &AERNPlayerController::StatusOpen);
				EnhancedInputComponent->BindAction(StatusAction, ETriggerEvent::Completed, this, &AERNPlayerController::StatusClose);
			}
			
			if (MinimapAction)
			{
				EnhancedInputComponent->BindAction(MinimapAction, ETriggerEvent::Started, this,
													&AERNPlayerController::ToggleMinimap);
			}

			if (PauseAction)
			{
				EnhancedInputComponent->BindAction(PauseAction, ETriggerEvent::Started, this,
													&AERNPlayerController::TogglePauseMenu);
			}
		}
	}
}

bool AERNPlayerController::ShouldUseTouchControls() const
{
	// are we on a mobile platform? Should we force touch?
	return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}

bool AERNPlayerController::IsPlayerAlive() const
{
	const AProjectERNCharacter* PlayerCharacter = Cast<AProjectERNCharacter>(GetPawn());
	return !PlayerCharacter || PlayerCharacter->IsAlive();
}

void AERNPlayerController::ToggleReady()
{
	AERNPlayerState* PS = GetPlayerState<AERNPlayerState>();
	if (PS)
	{
		// 현재 준비 상태를 토글해서 서버에 전송
		PS->Server_SetReady(!PS->bIsReady);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("ToggleReady called but PlayerState is null"));
	}
}

void AERNPlayerController::FlushPressedKeys()
{
	// UI 매니저가 열려있는 상태로 전환 중이거나 유지 중일 때는 입력을 끊지 않도록 방어
	if (const ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (UERNUIManagerSubsystem* UIManager = LocalPlayer->GetSubsystem<UERNUIManagerSubsystem>())
		{
			if (UIManager->GetActiveUIType() != EERNUIType::None)
			{
				return;
			}
		}
	}

	Super::FlushPressedKeys();
}

void AERNPlayerController::CheckAndFixCharacterType()
{
	if (!IsLocalPlayerController())
	{
		return;
	}

	// 10초 경과 체크
	float ElapsedTime = GetWorld()->GetTimeSeconds() - CharacterTypeCheckStartTime;
	if (ElapsedTime > 10.0f)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CheckAndFixCharacterType] Timeout (10 seconds). Stopping check."));
		GetWorld()->GetTimerManager().ClearTimer(CharacterTypeCheckTimerHandle);
		return;
	}

	UERNGameInstance* GI = GetGameInstance<UERNGameInstance>();
	AERNPlayerState* PS = GetPlayerState<AERNPlayerState>();

	if (!GI || !PS)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CheckAndFixCharacterType] GI or PS is null. GI: %s, PS: %s. Retrying..."),
		       GI ? TEXT("Valid") : TEXT("NULL"), PS ? TEXT("Valid") : TEXT("NULL"));
		return; // 타이머가 계속 돌면서 재시도
	}

	ECharacterType SavedType = GI->GetPlayerCharacterType();

	UE_LOG(LogTemp, Warning, TEXT("[CheckAndFixCharacterType] SavedType in GI: %d, Current PS CharacterType: %d"),
		static_cast<int32>(SavedType), static_cast<int32>(PS->CharacterType));

	// GameInstance에 저장된 타입이 있고, PlayerState와 다르면 재스폰
	if (SavedType != ECharacterType::None && SavedType != PS->CharacterType)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CheckAndFixCharacterType] Character type mismatch! Requesting change from %d to %d"),
			static_cast<int32>(PS->CharacterType), static_cast<int32>(SavedType));

		PS->Server_ChangeCharacterClass(SavedType);

		// 재스폰 요청 후 타이머 정지
		GetWorld()->GetTimerManager().ClearTimer(CharacterTypeCheckTimerHandle);
		UE_LOG(LogTemp, Log, TEXT("[CheckAndFixCharacterType] Character respawn requested. Stopping check."));
	}
	else if (SavedType != ECharacterType::None && SavedType == PS->CharacterType)
	{
		// 타입이 일치하면 성공 - 타이머 정지
		UE_LOG(LogTemp, Log, TEXT("[CheckAndFixCharacterType] Character type is correct: %d. Stopping check."), static_cast<int32>(PS->CharacterType));
		GetWorld()->GetTimerManager().ClearTimer(CharacterTypeCheckTimerHandle);
	}
	else
	{
		// SavedType이 None인 경우 - 로비에서 선택 안 한 경우
		UE_LOG(LogTemp, Log, TEXT("[CheckAndFixCharacterType] No saved character type. Using default."));
	}
}

void AERNPlayerController::TryInteract()
{
	if (!IsPlayerAlive())
	{
		return;
	}
	
	if (const ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (const UERNUIManagerSubsystem* UIManager = LocalPlayer->GetSubsystem<UERNUIManagerSubsystem>())
		{
			EERNUIType ActiveUI = UIManager->GetActiveUIType();
			// 인벤토리나 미니맵 등 이동이 가능한 UI 상태에서는 상호작용도 허용
			if (ActiveUI != EERNUIType::None && ActiveUI != EERNUIType::Inventory && ActiveUI != EERNUIType::Minimap)
			{
				return;
			}
		}
	}
	
	if (CurrentInteractableActor.IsValid())
	{
		if (IInteractable* Interactable = Cast<IInteractable>(CurrentInteractableActor.Get()))
		{
			if (!Interactable->Execute_CanInteract(CurrentInteractableActor.Get()))
			{
				return;
			}

			switch (Interactable->Execute_GetInteractionExecutionPolicy(CurrentInteractableActor.Get()))
			{
			case EInteractionExecutionPolicy::LocalOnly:
				Interactable->Execute_Interact(CurrentInteractableActor.Get(), this);
				break;
			case EInteractionExecutionPolicy::ServerAuthority:
				Server_TryInteract(CurrentInteractableActor.Get());
				break;
			}
		}
	}
}

void AERNPlayerController::Client_CloseInteractableWidgetsForPortal_Implementation()
{
	for (TObjectIterator<UERNInteractableWidget> It; It; ++It)
	{
		if (It->GetWorld() == GetWorld() && It->IsInViewport())
		{
			It->BP_PlayCloseAnimation();
			It->NotifyCloseAnimationFinished();
		}
	}
	
	if (const ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (UERNUIManagerSubsystem* UIManager = LocalPlayer->GetSubsystem<UERNUIManagerSubsystem>())
		{
			UIManager->ResetUIState();
		}
	}
	
	SetInputMode(FInputModeGameOnly());
	SetShowMouseCursor(false);
}

void AERNPlayerController::Server_RequestDHRollReward_Implementation(ADHRollActor* RollActor)
{
	if (!IsValid(RollActor))
	{
		return;
	}
	
	RollActor->SpawnRewardForPlayer(this);
}

void AERNPlayerController::Client_ResetInteractionInputState_Implementation()
{
	ClearCurrentInteractable();
	
	if (const ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (UERNUIManagerSubsystem* UIManager = LocalPlayer->GetSubsystem<UERNUIManagerSubsystem>())
		{
			UIManager->ResetUIState();
		}
	}
	
	FInputModeGameOnly InputMode;
	SetInputMode(InputMode);
	bShowMouseCursor  = false;
}

void AERNPlayerController::Server_TryInteract_Implementation(AActor* InteractableActor)
{
	if (!IsPlayerAlive() || !IsValid(InteractableActor) || !InteractableActor->Implements<UInteractable>())
	{
		return;
	}

	if (IInteractable::Execute_CanInteract(InteractableActor))
	{
		IInteractable::Execute_Interact(InteractableActor, this);
	}
}

void AERNPlayerController::InventoryOpen()
{
	if (!InventoryWidget)
	{
		return;
	}
	
	if (!IsPlayerAlive())
	{
		return;
	}
	
	// UI 매니저를 통한 상태 관리
	UERNUIManagerSubsystem* UIManager = ULocalPlayer::GetSubsystem<UERNUIManagerSubsystem>(GetLocalPlayer());
	
	if (InventoryWidget->GetVisibility() == ESlateVisibility::Hidden)
	{
		// 다른 UI가 열려있으면 인벤토리 열기 거부
		if (UIManager && !UIManager->RequestOpenUI(EERNUIType::Inventory))
		{
			return;
		}
		
		InventoryWidget->SetVisibility(ESlateVisibility::Visible);
		
		FInputModeGameAndUI InputMode;
		InputMode.SetWidgetToFocus(InventoryWidget->TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetHideCursorDuringCapture(true);
		SetInputMode(InputMode);
		bShowMouseCursor = true;
		
		if (UERNInventoryWidget* ERNInventoryWidget = Cast<UERNInventoryWidget>(InventoryWidget))
		{
			ERNInventoryWidget->PlayOpenAnimation();
		}
	}
}

void AERNPlayerController::InventoryClose()
{
	if (!InventoryWidget)
	{
		return;
	}
	
	UERNUIManagerSubsystem* UIManager = ULocalPlayer::GetSubsystem<UERNUIManagerSubsystem>(GetLocalPlayer());
	
	if (InventoryWidget->GetVisibility() == ESlateVisibility::Visible)
	{
		// UI 매니저에 닫힘 알림
		if (UIManager)
		{
			UIManager->CloseActiveUI();
		}
		
		InventoryWidget->SetVisibility(ESlateVisibility::Hidden);
		
		FInputModeGameOnly InputMode;
		SetInputMode(InputMode);
		bShowMouseCursor = false;
	}
}

void AERNPlayerController::StatusOpen()
{
	if (!StatusWidget) return;
	if (!IsPlayerAlive()) return;

	// 다른 전체화면 UI(인벤토리, 상점, 미니맵 등)가 열려있다면 탭 상태창을 띄우지 않음
	if (const ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (const UERNUIManagerSubsystem* UIManager = LocalPlayer->GetSubsystem<UERNUIManagerSubsystem>())
		{
			if (UIManager->GetActiveUIType() != EERNUIType::None)
			{
				return;
			}
		}
	}

	if (StatusWidget->GetVisibility() == ESlateVisibility::Hidden || StatusWidget->GetVisibility() == ESlateVisibility::Collapsed)
	{
		StatusWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		StatusWidget->RefreshAllAttributes();
	}
}

void AERNPlayerController::StatusClose()
{
	if (!StatusWidget) return;

	if (StatusWidget->GetVisibility() != ESlateVisibility::Hidden && StatusWidget->GetVisibility() != ESlateVisibility::Collapsed)
	{
		StatusWidget->SetVisibility(ESlateVisibility::Hidden);
	}
}

void AERNPlayerController::TogglePauseMenu()
{
	if (!IsLocalPlayerController())
	{
		return;
	}

	UERNUIManagerSubsystem* UIManager = ULocalPlayer::GetSubsystem<UERNUIManagerSubsystem>(GetLocalPlayer());

	// 만약 일시정지 메뉴가 이미 열려있다면 닫기
	if (UIManager && UIManager->GetActiveUIType() == EERNUIType::PauseMenu)
	{
		ClosePauseMenu();
		return;
	}

	// 만약 다른 UI가 열려있다면 (인벤토리 등), 
	// 열려있는 해당 UI가 포커스를 잃었더라도 무조건 닫히도록 모든 InteractableWidget에 닫기 명령을 내림
	if (UIManager && UIManager->GetActiveUIType() != EERNUIType::None)
	{
		for (TObjectIterator<UERNInteractableWidget> It; It; ++It)
		{
			if (It->GetWorld() == GetWorld() && It->IsInViewport())
			{
				It->BP_PlayCloseAnimation();
			}
		}
		return;
	}

	// 일시정지 메뉴 열기를 UI 매니저에 요청 (실패 시 반환)
	if (UIManager && !UIManager->RequestOpenUI(EERNUIType::PauseMenu))
	{
		return;
	}

	if (!PausedWidget && PausedWidgetClass)
	{
		PausedWidget = CreateWidget<UUserWidget>(this, PausedWidgetClass);
	}

	if (!PausedWidget)
	{
		if (UIManager) UIManager->CloseActiveUI();
		return;
	}

	// 코옵(네트워크)에서는 실제 일시정지가 불가 — 메뉴 오버레이로만 동작
	if (!PausedWidget->IsInViewport())
	{
		PausedWidget->AddToViewport(500);

		FInputModeGameAndUI InputMode;
		InputMode.SetWidgetToFocus(PausedWidget->TakeWidget());
		SetInputMode(InputMode);
		bShowMouseCursor = true;
	}
}

void AERNPlayerController::ClosePauseMenu()
{
	if (PausedWidget && PausedWidget->IsInViewport())
	{
		PausedWidget->RemoveFromParent();

		FInputModeGameOnly InputMode;
		SetInputMode(InputMode);
		bShowMouseCursor = false;
	}

	UERNUIManagerSubsystem* UIManager = ULocalPlayer::GetSubsystem<UERNUIManagerSubsystem>(GetLocalPlayer());
	if (UIManager && UIManager->GetActiveUIType() == EERNUIType::PauseMenu)
	{
		UIManager->CloseActiveUI();
	}
}

void AERNPlayerController::ShowEntranceWidget(const FText& EntranceText)
{
	if (!IsLocalPlayerController() || !EntranceWidgetClass)
	{
		return;
	}

	// 매번 새로 생성 — 위젯이 애니메이션 후 스스로 RemoveFromParent 하므로
	// 재진입 시 새 인스턴스로 애니메이션을 다시 재생
	EntranceWidget = CreateWidget<UERNEntranceWidget>(this, EntranceWidgetClass);
	if (!EntranceWidget)
	{
		return;
	}

	EntranceWidget->AddToViewport(50);

	// 텍스트블록에 반영 (숨김/애니메이션/RemoveFromParent는 위젯 BP 내부에서 처리)
	EntranceWidget->SetEntranceText(EntranceText);
}

void AERNPlayerController::Client_ShowDamageText_Implementation(AActor* TargetActor, FVector Location, float Damage)
{
	if (!DamageTextActorClass || !GetWorld()) return;

	const FVector AnchorLocation = Location + FVector(0.f, 0.f, 100.f);

	// 같은 적에 이미 떠 있는 텍스트가 있으면 새로 만들지 않고 누적 갱신 (겹침 방지)
	if (TargetActor)
	{
		if (TWeakObjectPtr<AERNDamageTextActor>* Found = ActiveDamageTexts.Find(TargetActor))
		{
			if (AERNDamageTextActor* Existing = Found->Get())
			{
				// 적 위치로 재고정 후 데미지 누적
				Existing->SetActorLocation(AnchorLocation);
				Existing->AddDamage(Damage);
				return;
			}
		}
	}

	// 신규 스폰 — 좌우/앞뒤 랜덤 오프셋으로 적별 위치 살짝 분산
	const float RandX = FMath::FRandRange(-DamageTextRandomOffset, DamageTextRandomOffset);
	const float RandY = FMath::FRandRange(-DamageTextRandomOffset, DamageTextRandomOffset);
	const FVector TextSpawnLocation = AnchorLocation + FVector(RandX, RandY, 0.f);

	AERNDamageTextActor* DamageTextActor = GetWorld()->SpawnActor<AERNDamageTextActor>(
		DamageTextActorClass, TextSpawnLocation, FRotator::ZeroRotator);

	if (DamageTextActor)
	{
		DamageTextActor->Initialize(Damage);

		// 적별 활성 텍스트로 등록 (무효 항목은 Add가 덮어씀)
		if (TargetActor)
		{
			ActiveDamageTexts.Add(TargetActor, DamageTextActor);
		}
	}
}

void AERNPlayerController::Client_PlayCameraShake_Implementation(TSubclassOf<UCameraShakeBase> ShakeClass, float Scale)
{
	if (!ShakeClass || !PlayerCameraManager) return;
	
	PlayerCameraManager->StartCameraShake(ShakeClass, 1.f);
	
	// 콘솔 진동
	const float RumbleIntensity = FMath::Clamp(Scale, 0.f, 1.f);
	const float RumbleDuration = FMath::Clamp(Scale, 0.2f, 0.7f);;

	if (RumbleIntensity > 0.05f && IsLocalController())
	{
		PlayDynamicForceFeedback(
			RumbleIntensity,
			RumbleDuration,
			true, true, true, true,
			EDynamicForceFeedbackAction::Start
		);
	}
}

void AERNPlayerController::Client_StartFadeIn_Implementation(float Duration)
{
	if (!PlayerCameraManager) return;

	// 1.0(검은화면) → 0.0(투명) 페이드. 인트로 시작 직전에 미리 검은 화면이어야 자연스러움
	PlayerCameraManager->StartCameraFade(1.f, 0.f, Duration, FLinearColor::Black, false, false);
}

void AERNPlayerController::Client_ShowIntroTitleWidget_Implementation()
{
	UERNGameInstance* GameInst = Cast<UERNGameInstance>(GetGameInstance());
	if (!GameInst) return;

	TSubclassOf<UUserWidget> WidgetClass = GameInst->GetIntroTitleWidgetClass();
	if (!WidgetClass) return;

	// 위젯은 Construct에서 자체 UMG Animation을 재생하고 종료 시 RemoveFromParent
	UUserWidget* IntroTitleWidget = CreateWidget<UUserWidget>(this, WidgetClass);
	if (IntroTitleWidget)
	{
		IntroTitleWidget->AddToViewport(100);
	}
}

void AERNPlayerController::UpdateLockOnMarker(bool bVisible, FVector2D ScreenPosition)
{
	if (!bVisible)
	{
		if (LockOnMarkerWidget)
		{
			LockOnMarkerWidget->SetVisibility(ESlateVisibility::Collapsed);
		}
		return;
	}

	if (!LockOnMarkerWidgetClass)
	{
		return;
	}

	// 최초 1회 lazy 생성
	if (!LockOnMarkerWidget)
	{
		LockOnMarkerWidget = CreateWidget<UUserWidget>(this, LockOnMarkerWidgetClass);
		if (LockOnMarkerWidget)
		{
			LockOnMarkerWidget->AddToViewport(40); // HUD 군 안쪽 ZOrder
			// 피벗을 중앙으로 → 좌표가 점의 중심을 가리킴 (거리 무관 정확)
			LockOnMarkerWidget->SetAlignmentInViewport(FVector2D(0.5f, 0.5f));
		}
	}

	if (LockOnMarkerWidget)
	{
		LockOnMarkerWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
		// 픽셀 좌표 → DPI 스케일 제거(true)해야 뷰포트 레이아웃 공간과 일치
		LockOnMarkerWidget->SetPositionInViewport(ScreenPosition, true);
	}
}

void AERNPlayerController::Client_ShowBossHealthBar_Implementation(AERNBossCharacter* Boss)
{
	if (!Boss || !BossHealthBarWidgetClass) return;

	// 위젯이 없으면 생성
	if (!BossHealthBarWidget)
	{
		BossHealthBarWidget = CreateWidget<UERNBossHealthBarWidget>(this, BossHealthBarWidgetClass);
		if (BossHealthBarWidget)
		{
			BossHealthBarWidget->AddToViewport(0); // 가장 낮은 ZOrder — 다른 HUD 위젯들 아래에 표시
			RegisterHUDWidget(BossHealthBarWidget);
		}
	}

	if (BossHealthBarWidget)
	{
		BossHealthBarWidget->SetVisibility(ESlateVisibility::Visible);

		// 보스 정보 설정 (체력은 Client_UpdateBossHealthBar에서 즉시 업데이트)
		BossHealthBarWidget->SetBossInfo(Boss->BossName, 1.0f);
	}
}

void AERNPlayerController::Client_UpdateBossHealthBar_Implementation(float HealthPercent, float MyDamageDealt)
{
	if (BossHealthBarWidget)
	{
		BossHealthBarWidget->UpdateHealth(HealthPercent, MyDamageDealt);
	}
}

void AERNPlayerController::Client_HideBossHealthBar_Implementation()
{
	if (BossHealthBarWidget)
	{
		BossHealthBarWidget->SetVisibility(ESlateVisibility::Hidden);
		BossHealthBarWidget->ResetAccumulatedDamage();
	}
}

//-------------------------NightRainZone 밤의비 자기장 --------------------------------//
#pragma region NightRainZone
//서버 : 상태 변화가 있을 때만 RPC 전송
void AERNPlayerController::UpdateNightRainPostProcessState_ServerOnly(bool bShouldEnable)
{
	if (HasAuthority() == false)
	{
		return;
	}
	
	// 자기장 무시 상태라면 판정 생략
	if (bIgnoreNightRainZone_Server)
	{
		bShouldEnable = false;
	}
	
	if (bServerNightRainPostProcessEnabled == bShouldEnable)
	{
		return;
	}
	
	bServerNightRainPostProcessEnabled = bShouldEnable;
	Client_SetNightRainZonePostProcessEnabled(bShouldEnable);
}

void AERNPlayerController::SetIgnoreNightRainZone_ServerOnly(bool bIgnore)
{
	if (HasAuthority() == false)
	{
		return;
	}
	
	// UpdateNightRainPostProcessState_ServerOnly 를 여러번 호출하지 않도록 중복 호출 방지
	if (bIgnoreNightRainZone_Server == bIgnore)
	{
		return;
	}
	
	bIgnoreNightRainZone_Server = bIgnore;
	
	if (bIgnoreNightRainZone_Server)
	{
		UpdateNightRainPostProcessState_ServerOnly(false);
	}
}

// 클라이언트 : 자기 로컬 화면만 변경
void AERNPlayerController::Client_SetNightRainZonePostProcessEnabled_Implementation(bool bEnabled)
{
	if (IsLocalPlayerController() == false)
	{
		return;
	}
	
	if (bLocalNightRainPostProcessEnabled == bEnabled)
	{
		return;
	}
	
	bLocalNightRainPostProcessEnabled = bEnabled;
	SetNightRainZonePostProcessEnabled_Local(bEnabled);
}

// BlendWeight 값만 변경해서 가시성 On/Off
void AERNPlayerController::SetNightRainZonePostProcessEnabled_Local(bool bEnabled)
{
	TargetNightRainPostProcessBlendWeight = bEnabled ? 1.f : 0.f;
	
	GetWorldTimerManager().SetTimer(NightRainPostProcessBlendTimerHandle,
										this,
										&AERNPlayerController::TickNightRainZonePostProcessBlend,
										0.016f,
										true);
}

void AERNPlayerController::TickNightRainZonePostProcessBlend()
{
	if (GetWorld() == nullptr)
	{
		return;
	}
	
	CurrentNightRainPostProcessBlendWeight = FMath::FInterpTo(CurrentNightRainPostProcessBlendWeight, TargetNightRainPostProcessBlendWeight, GetWorld()->GetDeltaSeconds(), NightRainPostProcessInterpSpeed);
	
	SetNightRainPostProcessBlendWeight_Local(CurrentNightRainPostProcessBlendWeight);
	
	if (FMath::IsNearlyEqual(CurrentNightRainPostProcessBlendWeight, TargetNightRainPostProcessBlendWeight, 0.01f))
	{
		CurrentNightRainPostProcessBlendWeight = TargetNightRainPostProcessBlendWeight;
		SetNightRainPostProcessBlendWeight_Local(CurrentNightRainPostProcessBlendWeight);
		
		GetWorldTimerManager().ClearTimer(NightRainPostProcessBlendTimerHandle);
	}
}

// ===== 슬라이스 프리즈 화면 연출 (소유 클라 로컬) =====

void AERNPlayerController::Client_PlaySliceEffect_Implementation(float Duration)
{
	// 이미 진행 중인 연출이 있으면 깔끔하게 정리 후 재시작
	EndSliceEffectLocal();

	// 이동 + 시점(카메라) 입력 차단 (서버 MOVE_None과 별개로 로컬 예측 입력도 막음)
	SetIgnoreMoveInput(true);
	SetIgnoreLookInput(true);

	SliceEffectStartTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
	SliceEffectDuration = FMath::Max(Duration, 0.f);

	// 화면 변형 머티리얼을 슬라이스 포스트프로세스에 블렌더블로 주입
	const AProjectERNCharacter* ERNCharacter = Cast<AProjectERNCharacter>(GetPawn());
	UPostProcessComponent* SlicePP = ERNCharacter ? ERNCharacter->GetSlicePostProcessComponent() : nullptr;
	if (SliceMaterial && SlicePP)
	{
		SliceMID = UMaterialInstanceDynamic::Create(SliceMaterial, this);
		if (SliceMID)
		{
			SliceMID->SetScalarParameterValue(TEXT("SliceAmount"), 0.f);
			SlicePP->Settings.WeightedBlendables.Array.Empty();
			SlicePP->Settings.AddBlendable(SliceMID, 1.f);
		}
	}

	// Duration이 0 이하면 시각 효과 없이 즉시 종료(입력 복구)
	if (SliceEffectDuration <= 0.f)
	{
		EndSliceEffectLocal();
		return;
	}

	GetWorldTimerManager().SetTimer(
		SliceEffectTimerHandle, this, &AERNPlayerController::TickSliceEffect, 0.016f, true);
}

void AERNPlayerController::TickSliceEffect()
{
	if (GetWorld() == nullptr)
	{
		EndSliceEffectLocal();
		return;
	}

	// 시작 시각 기준 절대 경과 (프레임레이트/타이머 주기와 무관하게 정확)
	const float Elapsed = GetWorld()->GetTimeSeconds() - SliceEffectStartTime;

	// envelope: 빠르게 어긋남(Rise) → 유지(Hold) → 종료 직전 복귀(Fall)
	float Amount = SliceMaxOffset;
	const float FallStart = SliceEffectDuration - SliceFallTime;

	if (SliceRiseTime > 0.f && Elapsed < SliceRiseTime)
	{
		Amount = SliceMaxOffset * (Elapsed / SliceRiseTime);
	}
	else if (SliceFallTime > 0.f && Elapsed > FallStart)
	{
		const float FallAlpha = FMath::Clamp((SliceEffectDuration - Elapsed) / SliceFallTime, 0.f, 1.f);
		Amount = SliceMaxOffset * FallAlpha;
	}

	if (SliceMID)
	{
		SliceMID->SetScalarParameterValue(TEXT("SliceAmount"), Amount);
	}

	if (Elapsed >= SliceEffectDuration)
	{
		EndSliceEffectLocal();
	}
}

void AERNPlayerController::EndSliceEffectLocal()
{
	GetWorldTimerManager().ClearTimer(SliceEffectTimerHandle);

	// 이동 + 시점 입력 복구 (Client_PlaySliceEffect에서 건 차단 해제)
	SetIgnoreMoveInput(false);
	SetIgnoreLookInput(false);

	// 블렌더블 제거 + 파라미터 초기화
	if (const AProjectERNCharacter* ERNCharacter = Cast<AProjectERNCharacter>(GetPawn()))
	{
		if (UPostProcessComponent* SlicePP = ERNCharacter->GetSlicePostProcessComponent())
		{
			SlicePP->Settings.WeightedBlendables.Array.Empty();
		}
	}

	if (SliceMID)
	{
		SliceMID->SetScalarParameterValue(TEXT("SliceAmount"), 0.f);
		SliceMID = nullptr;
	}

	SliceEffectStartTime = 0.f;
	SliceEffectDuration = 0.f;
}

UPostProcessComponent* AERNPlayerController::FindNightRainPostProcessComponent() const
{
	const AProjectERNCharacter* ERNCharacter = Cast<AProjectERNCharacter>(GetPawn());
	return ERNCharacter ? ERNCharacter->GetNightRainPostProcessComponent() : nullptr;
}

void AERNPlayerController::SetNightRainPostProcessBlendWeight_Local(float BlendWeight)
{
	if (UPostProcessComponent* PostProcessComponent = FindNightRainPostProcessComponent())
	{
		PostProcessComponent->BlendWeight = BlendWeight;
	}
}
#pragma endregion

void AERNPlayerController::Client_CompleteChurchInteraction_Implementation(AChurch* Church)
{
	if (Church)
	{
		Church->CompleteInteractionLocally(GetCharacter());
	}
}

//------------------------Minimap 미니맵 --------------------------------//
#pragma region Minimap
void AERNPlayerController::ToggleMinimap()
{
	if (MinimapWidget == nullptr)
	{
		return;
	}
	
	if (MinimapWidget->GetVisibility() == ESlateVisibility::Visible)
	{
		MinimapClose();
		return;
	}
	
	MinimapOpen();
}

void AERNPlayerController::MinimapOpen()
{
	if (MinimapWidget == nullptr)
	{
		return;
	}

	// UI 매니저를 통한 상태 관리
	UERNUIManagerSubsystem* UIManager = ULocalPlayer::GetSubsystem<UERNUIManagerSubsystem>(GetLocalPlayer());

	if (MinimapWidget->GetVisibility() == ESlateVisibility::Collapsed)
	{
		// 다른 UI가 열려있다면 모두 닫고 미니맵 활성화
		if (UIManager && !UIManager->RequestOpenUI(EERNUIType::Minimap))
		{
			return;
		}
		
		MinimapWidget->SetVisibility(ESlateVisibility::Visible);

		FInputModeGameAndUI InputMode;
		InputMode.SetWidgetToFocus(MinimapWidget->TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetHideCursorDuringCapture(true);
		SetInputMode(InputMode);
		bShowMouseCursor = true;

		if (UERNMinimapWidget* ERNMinimapWidget = Cast<UERNMinimapWidget>(MinimapWidget))
		{
			ERNMinimapWidget->RefreshStaticMarkers();
			ERNMinimapWidget->StartDynamicMinimapRefresh();
			ERNMinimapWidget->PlayOpenAnimation();
			ERNMinimapWidget->StartGamepadMoveRefresh();
		}
	}
}

void AERNPlayerController::MinimapClose()
{
	if (MinimapWidget == nullptr)
	{
		return;
	}

	UERNUIManagerSubsystem* UIManager = ULocalPlayer::GetSubsystem<UERNUIManagerSubsystem>(GetLocalPlayer());

	if (MinimapWidget->GetVisibility() == ESlateVisibility::Visible)
	{
		// UI 매니저에 닫힘 알림
		if (UIManager)
		{
			UIManager->CloseActiveUI();
		}

		// 플레이어 마커 타이머 정리
		if (UERNMinimapWidget* ERNMinimapWidget = Cast<UERNMinimapWidget>(MinimapWidget))
		{
			ERNMinimapWidget->StopDynamicMinimapRefresh();
			ERNMinimapWidget->StopGamepadMoveRefresh();
		}
		
		MinimapWidget->SetVisibility(ESlateVisibility::Collapsed);

		FInputModeGameOnly InputMode;
		SetInputMode(InputMode);
		bShowMouseCursor = false;
	}
}

void AERNPlayerController::Server_RequestCreateMinimapPin_Implementation(FVector WorldLocation)
{
	if (MinimapPinClass == nullptr)
	{
		return;
	}
	
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}
	
	AERNPlayerState* ERNPlayerState = GetPlayerState<AERNPlayerState>();
	if (ERNPlayerState == nullptr)
	{
		return;
	}
	
	// 일정거리 이내의 본인 핑 삭제
	constexpr float ToggleDistance = 500.f;
	const float ToggleDistanceSq = FMath::Square(ToggleDistance);

	for (TActorIterator<AERNMinimapPinPoint> It(World); It; ++It)
	{
		AERNMinimapPinPoint* Pin = *It;
		if (!IsValid(Pin))
		{
			continue;
		}

		if (Pin->GetOwnerPlayerState() != PlayerState)
		{
			continue;
		}

		if (FVector::DistSquared2D(Pin->GetActorLocation(), WorldLocation) <= ToggleDistanceSq)
		{
			Pin->Destroy();
			return;
		}
	}
	
	// 기존 핀 삭제
	DestroyOwnedMinimapPins_ServerOnly();
	
	// 이후 새로 생성
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = GetPawn();
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AERNMinimapPinPoint* NewPin = World->SpawnActor<AERNMinimapPinPoint>(
		MinimapPinClass,
		WorldLocation,
		FRotator::ZeroRotator,
		SpawnParams);

	if (NewPin == nullptr)
	{
		return;
	}

	NewPin->InitializePin(ERNPlayerState->GetMinimapPinIconType(), ERNPlayerState);
}

void AERNPlayerController::Server_RequestRemoveMinimapPin_Implementation(AERNMinimapPinPoint* PinActor)
{
	if (IsValid(PinActor) == false)
	{
		return;
	}
	
	// 본인 핀만 제거
	if (PinActor->GetOwnerPlayerState() != PlayerState)
	{
		return;
	}
	
	PinActor->Destroy();
}

void AERNPlayerController::DestroyOwnedMinimapPins_ServerOnly()
{
	UWorld* World = GetWorld();
	if (World == nullptr || PlayerState == nullptr)
	{
		return;
	}
	
	// 본인의 모든 핀 청소
	for(TActorIterator<AERNMinimapPinPoint> It(World); It; ++It)
	{
		AERNMinimapPinPoint* Pin = *It;
		
		if (IsValid(Pin) == false)
		{
			continue;
		}
		
		if (Pin->GetOwnerPlayerState() == PlayerState)
		{
			Pin->Destroy();
		}
	}
	
}
#pragma endregion


// ===== 채팅 시스템 =====

void AERNPlayerController::Server_SendChat_Implementation(const FString& Message)
{
	if (Message.IsEmpty()) return;

	// 길이 제한 200자
	const FString TrimmedMessage = Message.Left(200);

	// 닉네임 + 색상 결정 (PlayerId 기반 팔레트 인덱싱)
	FString Sender = TEXT("Player");
	FLinearColor SenderColor = FLinearColor::White;
	if (AERNPlayerState* PS = GetPlayerState<AERNPlayerState>())
	{
		if (!PS->PlayerNickname.IsEmpty())
		{
			Sender = PS->PlayerNickname;
		}

		if (ChatColorPalette.Num() > 0)
		{
			const int32 Index = FMath::Abs(PS->GetPlayerId()) % ChatColorPalette.Num();
			SenderColor = ChatColorPalette[Index];
		}
	}

	// 모든 PlayerController에 Client RPC 전송
	UWorld* World = GetWorld();
	if (!World) return;

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		if (AERNPlayerController* TargetPC = Cast<AERNPlayerController>(It->Get()))
		{
			TargetPC->Client_ReceiveChat(Sender, TrimmedMessage, SenderColor);
		}
	}
}

void AERNPlayerController::Client_ReceiveChat_Implementation(const FString& Sender, const FString& Message, FLinearColor SenderColor)
{
	// BP가 ChatWidget의 AddMessage 노드 호출 (최대 20개)
	OnReceiveChatMessage(Sender, Message, SenderColor);
}

void AERNPlayerController::ShowEndScreen(bool bVictory)
{
	if (!IsLocalPlayerController())
	{
		return;
	}

	// 게임 종료 화면에선 게임플레이 HUD(나침반/파티/미니맵 등) 숨김
	for (const TObjectPtr<UUserWidget>& HUDWidget : ManagedHUDWidgets)
	{
		if (IsValid(HUDWidget))
		{
			HUDWidget->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	TSubclassOf<UUserWidget> WidgetClass = bVictory ? VictoryBannerWidgetClass : DefeatBannerWidgetClass;
	if (!WidgetClass)
	{
		return;
	}

	// 배너 위젯(BP)이 일정시간 뒤 전과 위젯으로 전환
	if (UUserWidget* Banner = CreateWidget<UUserWidget>(this, WidgetClass))
	{
		Banner->AddToViewport(200);
	}
}

void AERNPlayerController::Server_RequestReturnToLobby_Implementation()
{
	if (AERNGameState* GS = GetWorld()->GetGameState<AERNGameState>())
	{
		GS->MarkReturnReady(GetPlayerState<AERNPlayerState>());
	}
}

void AERNPlayerController::Server_CancelReturnToLobby_Implementation()
{
	if (AERNGameState* GS = GetWorld()->GetGameState<AERNGameState>())
	{
		GS->UnmarkReturnReady(GetPlayerState<AERNPlayerState>());
	}
}

void AERNPlayerController::RefreshSkillCoolPanel() const
{
	if (!IsLocalPlayerController() || !SkillCoolPanelWidget)
	{
		return;
	}

	SkillCoolPanelWidget->RefreshFromCurrentCharacter();
}

#pragma region PlayerRespawn
// ===== Player Respawn =====
bool AERNPlayerController::GetStreamingSourcesInternal(TArray<FWorldPartitionStreamingSource>& OutStreamingSources) const
{
	const bool bHasDefaultSource = Super::GetStreamingSourcesInternal(OutStreamingSources);

	if (bRespawnPreloadStreamingActive)
	{
		FWorldPartitionStreamingSource& Source = OutStreamingSources.AddDefaulted_GetRef();

		Source.Name = FName(*FString::Printf(TEXT("%s_RespawnPreload"), *GetName()));
		Source.Location = RespawnPreloadLocation;
		Source.Rotation = FRotator::ZeroRotator;
		Source.TargetState = EStreamingSourceTargetState::Activated;
		Source.bBlockOnSlowLoading = true;
		Source.Priority = EStreamingSourcePriority::Highest;
		Source.DebugColor = FColor::Cyan;
		Source.bRemote = !IsLocalController();

		FStreamingSourceShape Shape;
		Shape.bUseGridLoadingRange = false;
		Shape.Radius = RespawnPreloadRadius;

		Source.Shapes.Add(Shape);
	}

	return bHasDefaultSource || bRespawnPreloadStreamingActive;
}

void AERNPlayerController::BeginRespawnPreloadStreaming(const FVector& Location, float Radius)
{
	bRespawnPreloadStreamingActive = true;
	RespawnPreloadLocation = Location;
	RespawnPreloadRadius = FMath::Max(Radius, 1000.f);

	if (HasAuthority())
	{
		bRespawnClientPreloadReady_Server = IsLocalController();
	}
}

void AERNPlayerController::EndRespawnPreloadStreaming()
{
	bRespawnPreloadStreamingActive = false;

	if (HasAuthority())
	{
		bRespawnClientPreloadReady_Server = false;
	}

	GetWorldTimerManager().ClearTimer(RespawnPreloadReadyCheckTimerHandle);
}

bool AERNPlayerController::IsRespawnPreloadStreamingCompleted() const
{
	if (!bRespawnPreloadStreamingActive)
	{
		return true;
	}

	if (UWorld* World = GetWorld())
	{
		if (UWorldPartitionSubsystem* WorldPartitionSubsystem = World->GetSubsystem<UWorldPartitionSubsystem>())
		{
			return WorldPartitionSubsystem->IsStreamingCompleted(this);
		}
	}

	return true;
}

void AERNPlayerController::Client_BeginRespawnPreloadStreaming_Implementation(FVector Location, float Radius)
{
	BeginRespawnPreloadStreaming(Location, Radius);

	GetWorldTimerManager().ClearTimer(RespawnPreloadReadyCheckTimerHandle);
	GetWorldTimerManager().SetTimer(
		RespawnPreloadReadyCheckTimerHandle,
		this,
		&AERNPlayerController::CheckRespawnPreloadStreamingReady_Local,
		0.1f,
		true,
		0.f);
}

void AERNPlayerController::Client_EndRespawnPreloadStreaming_Implementation()
{
	EndRespawnPreloadStreaming();
}

void AERNPlayerController::CheckRespawnPreloadStreamingReady_Local()
{
	if (!bRespawnPreloadStreamingActive)
	{
		GetWorldTimerManager().ClearTimer(RespawnPreloadReadyCheckTimerHandle);
		return;
	}

	if (!IsRespawnPreloadStreamingCompleted())
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(RespawnPreloadReadyCheckTimerHandle);
	Server_NotifyRespawnPreloadStreamingReady();
}

void AERNPlayerController::Server_NotifyRespawnPreloadStreamingReady_Implementation()
{
	if (!bRespawnPreloadStreamingActive)
	{
		return;
	}

	bRespawnClientPreloadReady_Server = true;
}

#pragma endregion PlayerRespawn

#pragma region SelfDownedStatusWidget

void AERNPlayerController::InitializeSelfDownedStatusWidget()
{
	if (!IsLocalPlayerController() || SelfDownedStatusWidget || !SelfDownedStatusWidgetClass)
	{
		return;
	}

	SelfDownedStatusWidget = CreateWidget<UERNDownedStatusWidget>(this, SelfDownedStatusWidgetClass);
	if (!SelfDownedStatusWidget)
	{
		return;
	}

	SelfDownedStatusWidget->AddToViewport(150);
	RegisterHUDWidget(SelfDownedStatusWidget);
}

void AERNPlayerController::RefreshSelfDownedStatusWidget()
{
	if (!IsLocalPlayerController())
	{
		return;
	}

	InitializeSelfDownedStatusWidget();

	if (SelfDownedStatusWidget)
	{
		SelfDownedStatusWidget->SetObservedCharacter(Cast<AProjectERNCharacter>(GetPawn()));
	}
}

#pragma endregion SelfDownedStatusWidget