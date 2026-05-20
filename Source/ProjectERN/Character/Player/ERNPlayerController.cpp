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
#include "UI/ERNUIManagerSubsystem.h"
#include "UI/ERNDamageTextActor.h"
#include "UI/ERNBossHealthBarWidget.h"
#include "Character/Enemy/ERNBossCharacter.h"
#include "Camera/CameraShakeBase.h"

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
				if (UERNInteractableWidget* InteractableInventoryWidget = Cast<UERNInteractableWidget>(InventoryWidget))
				{
					InteractableInventoryWidget->OnWidgetClosed.AddUniqueDynamic(this, &AERNPlayerController::InventoryClose);
				}
				InventoryWidget->AddToViewport();
				RefreshInventoryWidget();
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

void AERNPlayerController::AcknowledgePossession(class APawn* P)
{
	Super::AcknowledgePossession(P);
	
	RefreshInventoryWidget();
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
				EnhancedInputComponent->BindAction(InventoryAction, ETriggerEvent::Started, this, &AERNPlayerController::InventoryOpen);
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

void AERNPlayerController::InventoryOpen()
{
	if (!InventoryWidget)
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

void AERNPlayerController::Client_ShowDamageText_Implementation(FVector Location, float Damage)
{
	if (!DamageTextActorClass || !GetWorld()) return;

	// 좌우/앞뒤 랜덤 오프셋 (겹침 방지) + 위로 100 오프셋
	const float RandX = FMath::FRandRange(-DamageTextRandomOffset, DamageTextRandomOffset);
	const float RandY = FMath::FRandRange(-DamageTextRandomOffset, DamageTextRandomOffset);
	FVector TextSpawnLocation = Location + FVector(RandX, RandY, 100.f);

	AERNDamageTextActor* DamageTextActor = GetWorld()->SpawnActor<AERNDamageTextActor>(
		DamageTextActorClass, TextSpawnLocation, FRotator::ZeroRotator);

	if (DamageTextActor)
	{
		DamageTextActor->Initialize(Damage);
	}
}

void AERNPlayerController::Client_PlayCameraShake_Implementation(TSubclassOf<UCameraShakeBase> ShakeClass, float Scale)
{
	if (!ShakeClass || !PlayerCameraManager) return;

	PlayerCameraManager->StartCameraShake(ShakeClass, Scale);
}

void AERNPlayerController::Client_StartFadeIn_Implementation(float Duration)
{
	if (!PlayerCameraManager) return;

	// 1.0(검은화면) → 0.0(투명) 페이드. 인트로 시작 직전에 미리 검은 화면이어야 자연스러움
	PlayerCameraManager->StartCameraFade(1.f, 0.f, Duration, FLinearColor::Black, false, false);
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
			BossHealthBarWidget->AddToViewport(100); // 높은 ZOrder로 최상위 표시
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
