// Copyright Epic Games, Inc. All Rights Reserved.

#include "Actors/ERNAccountStatActor.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Blueprint/UserWidget.h"
#include "Character/Player/ERNPlayerController.h"
#include "UI/ERNInteractableWidget.h"
#include "UI/ERNUIManagerSubsystem.h"

AERNAccountStatActor::AERNAccountStatActor()
{
	PrimaryActorTick.bCanEverTick = false;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	RootComponent = MeshComponent;

	InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionSphere"));
	InteractionSphere->SetupAttachment(RootComponent);
	InteractionSphere->SetSphereRadius(200.f);
	InteractionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionSphere->SetCollisionResponseToAllChannels(ECR_Overlap);
	InteractionSphere->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);

	InteractionPromptWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("InteractionPromptWidget"));
	InteractionPromptWidget->SetupAttachment(RootComponent);
	InteractionPromptWidget->SetWidgetSpace(EWidgetSpace::World);
	InteractionPromptWidget->SetVisibility(false);
}

void AERNAccountStatActor::BeginPlay()
{
	Super::BeginPlay();
}

void AERNAccountStatActor::Interact_Implementation(APlayerController* PlayerController)
{
	if (!bIsActive) return;

	AERNPlayerController* ERNPC = Cast<AERNPlayerController>(PlayerController);
	if (!ERNPC) return;

	if (ActiveWidget) return;

	// UI 매니저 게이트: 다른 UI가 열려있으면 차단
	ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer();
	if (!LocalPlayer) return;

	UERNUIManagerSubsystem* UIManager = LocalPlayer->GetSubsystem<UERNUIManagerSubsystem>();
	if (!UIManager) return;

	if (!UIManager->RequestOpenUI(EERNUIType::AccountStat))
	{
		return;
	}

	if (!AccountStatWidgetClass)
	{
		return;
	}

	ActiveWidget = CreateWidget<UUserWidget>(PlayerController, AccountStatWidgetClass);
	if (ActiveWidget)
	{
		ActiveWidget->AddToViewport();
		ActiveWidget->SetFocus();

		if (UERNInteractableWidget* InteractableWidget = Cast<UERNInteractableWidget>(ActiveWidget))
		{
			InteractableWidget->OnWidgetClosed.AddDynamic(this, &AERNAccountStatActor::HandleWidgetClosed);
		}
	}

	// 마우스 커서 표시 + 입력 모드 변경
	PlayerController->SetShowMouseCursor(true);
	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	PlayerController->SetInputMode(InputMode);
}

bool AERNAccountStatActor::CanInteract_Implementation() const
{
	return bIsActive;
}

FText AERNAccountStatActor::GetInteractionText_Implementation() const
{
	return bIsActive ? InteractionPrompt : FText::FromString(TEXT("이용할 수 없음"));
}

void AERNAccountStatActor::EndInteract_Implementation(APlayerController* PlayerController)
{
	if (InteractionPromptWidget) InteractionPromptWidget->SetVisibility(false);

	if (ActiveWidget)
	{
		if (UERNInteractableWidget* IW = Cast<UERNInteractableWidget>(ActiveWidget))
		{
			IW->BP_PlayCloseAnimation();
		}
		else
		{
			ActiveWidget->RemoveFromParent();
			ActiveWidget = nullptr;

			if (AERNPlayerController* ERNPC = Cast<AERNPlayerController>(PlayerController))
			{
				ERNPC->SetInputMode(FInputModeGameOnly());
				ERNPC->SetShowMouseCursor(false);

				if (ULocalPlayer* LocalPlayer = ERNPC->GetLocalPlayer())
				{
					if (UERNUIManagerSubsystem* UIManager = LocalPlayer->GetSubsystem<UERNUIManagerSubsystem>())
					{
						UIManager->CloseActiveUI();
					}
				}
			}
		}
	}
}

void AERNAccountStatActor::HandleWidgetClosed()
{
	if (ActiveWidget)
	{
		ActiveWidget->RemoveFromParent();
		ActiveWidget = nullptr;
	}

	if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		PC->SetShowMouseCursor(false);
		PC->SetInputMode(FInputModeGameOnly());

		if (AERNPlayerController* ERNPC = Cast<AERNPlayerController>(PC))
		{
			if (ULocalPlayer* LocalPlayer = ERNPC->GetLocalPlayer())
			{
				if (UERNUIManagerSubsystem* UIManager = LocalPlayer->GetSubsystem<UERNUIManagerSubsystem>())
				{
					UIManager->CloseActiveUI();
				}
			}
		}
	}
}

void AERNAccountStatActor::ActivateInteract_Implementation() const
{
	if (InteractionPromptWidget) InteractionPromptWidget->SetVisibility(true);
}

EInteractionExecutionPolicy AERNAccountStatActor::GetInteractionExecutionPolicy_Implementation() const
{
	return EInteractionExecutionPolicy::LocalOnly;
}
