// Copyright Epic Games, Inc. All Rights Reserved.


#include "Character/Player/ERNPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "Blueprint/UserWidget.h"
#include "ProjectERN.h"
#include "Widgets/Input/SVirtualJoystick.h"
#include "Character/Player/ERNPlayerState.h"
#include "Core/ERNGameInstance.h"
#include "Interfaces/IInteractable.h"
#include "UI/ERNInventoryWidget.h"

void AERNPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// only spawn touch controls on local player controllers
	if (ShouldUseTouchControls() && IsLocalPlayerController())
	{
		// spawn the mobile controls widget
		MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);

		if (MobileControlsWidget)
		{
			// add the controls to the player screen
			MobileControlsWidget->AddToPlayerScreen(0);

		} else {

			UE_LOG(LogProjectERN, Error, TEXT("Could not spawn mobile controls widget."));

		}
	}

	// 현재 맵 이름 가져오기 (UI 생성용)
	FString CurrentMapName = GetWorld()->GetMapName();

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
				InventoryWidget->AddToViewport();
			}
		}
	}

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
				EnhancedInputComponent->BindAction(ReadyToggleAction, ETriggerEvent::Started, this, &AERNPlayerController::ToggleReady);
			}

			if (InteractAction)
			{
				EnhancedInputComponent->BindAction(InteractAction, ETriggerEvent::Started, this, &AERNPlayerController::TryInteract);
			}
			
			if (InventoryAction)
			{
				EnhancedInputComponent->BindAction(InventoryAction, ETriggerEvent::Started, this, &AERNPlayerController::ToggleInventory);
			}
		}
	}
}

bool AERNPlayerController::ShouldUseTouchControls() const
{
	// are we on a mobile platform? Should we force touch?
	return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
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
	UE_LOG(LogTemp, Warning, TEXT("TryInteract"));
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

void AERNPlayerController::Server_TryInteract_Implementation(AActor* InteractableActor)
{
	if (InteractableActor->Implements<UInteractable>())
	{
		if (IInteractable::Execute_CanInteract(InteractableActor))
		{
			IInteractable::Execute_Interact(InteractableActor, this);
		}
	}
}

void AERNPlayerController::ToggleInventory()
{
	if (!InventoryWidget)
	{
		return;
	}
	
	UE_LOG(LogTemp, Warning, TEXT("ToggleInventory"));
	
	if (InventoryWidget->GetVisibility() == ESlateVisibility::Hidden)
	{
		InventoryWidget->SetVisibility(ESlateVisibility::Visible);
		
		FInputModeUIOnly InputMode;
		InputMode.SetWidgetToFocus(InventoryWidget->TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		SetInputMode(InputMode);
		bShowMouseCursor = true;
		
		Cast<UERNInventoryWidget>(InventoryWidget)->InitFocusSlotIndex();
	}
	else
	{
		InventoryWidget->SetVisibility(ESlateVisibility::Hidden);
		
		FInputModeGameOnly InputMode;
		SetInputMode(InputMode);
		bShowMouseCursor = false;
	}
}
