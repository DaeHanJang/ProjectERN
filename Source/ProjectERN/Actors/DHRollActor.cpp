// Fill out your copyright notice in the Description page of Project Settings.

#include "Actors/DHRollActor.h"

#include "Character/ERNCharacterBase.h"
#include "Character/Player/ERNPlayerController.h"
#include "Character/Player/ERNPlayerState.h"
#include "Character/Player/ProjectERNCharacter.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "GAS/ERNAttributeSet.h"
#include "Inventory/Item/ERNItemActor.h"
#include "Inventory/Item/Manager/ItemManagerSubsystem.h"
#include "UI/DHRollWidget.h"
#include "UI/ERNInteractableWidget.h"
#include "UI/ERNUIManagerSubsystem.h"

// Sets default values
ADHRollActor::ADHRollActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	
	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	SetRootComponent(Collision);
	Collision->InitSphereRadius(250.0f);
	Collision->SetCollisionProfileName(TEXT("OverlapAll"));
	Collision->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel1, ECollisionResponse::ECR_Ignore);
	
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(GetRootComponent());
	Mesh->SetCollisionProfileName(TEXT("BlockAll"));
	Mesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Overlap);
	
	Prompt = CreateDefaultSubobject<UWidgetComponent>(TEXT("PromptComponent"));
	Prompt->SetupAttachment(GetRootComponent());
	Prompt->SetWidgetSpace(EWidgetSpace::Screen);
	Prompt->SetVisibility(false);	
}

void ADHRollActor::BeginPlay()
{
	Super::BeginPlay();
	
	InitRoll();
}

EInteractionExecutionPolicy ADHRollActor::GetInteractionExecutionPolicy_Implementation() const
{
	return EInteractionExecutionPolicy::LocalOnly;
}

void ADHRollActor::ActivateInteract_Implementation() const
{
	if (Prompt && CanInteract_Implementation())
	{
		Prompt->SetVisibility(true);
	}
}

bool ADHRollActor::CanInteract_Implementation() const
{
	return !IsActorBeingDestroyed() && !PlayerControllersOverFlag.Contains(GetWorld()->GetFirstPlayerController()) && !PlayerControllersRewordFlag.Contains(GetWorld()->GetFirstPlayerController());
}

void ADHRollActor::Interact_Implementation(APlayerController* PlayerController)
{
	if (!PlayerController || !RollWidgetClass || RollWidget)
	{
		return;
	}
	
	if (ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer())
	{
		if (UERNUIManagerSubsystem* UIManager = LocalPlayer->GetSubsystem<UERNUIManagerSubsystem>())
		{
			if (!UIManager->RequestOpenUI(EERNUIType::DHRoll))
			{
				return;
			}
		}
	}
	
	RollWidget = CreateWidget<UUserWidget>(PlayerController, RollWidgetClass);
	
	if (RollWidget)
	{
		RollWidget->AddToViewport(100);
		
		if (UERNInteractableWidget* InteractableWidget = Cast<UERNInteractableWidget>(RollWidget))
		{
			InteractableWidget->OnWidgetClosed.RemoveDynamic(this, &ADHRollActor::Close);
			InteractableWidget->OnWidgetClosed.AddUniqueDynamic(this, &ADHRollActor::Close);
			
			if (UDHRollWidget* Widget = Cast<UDHRollWidget>(RollWidget))
			{
				Widget->OnRollClicked.RemoveDynamic(this, &ADHRollActor::Roll);
				Widget->OnRollClicked.AddUniqueDynamic(this, &ADHRollActor::Roll);
				Widget->OnRewardClicked.RemoveDynamic(this, &ADHRollActor::Reward);
				Widget->OnRewardClicked.AddUniqueDynamic(this, &ADHRollActor::Reward);
				if (PlayerControllersRewordGrade.Contains(PlayerController))
				{
					Widget->SetText(*PlayerControllersRewordGrade.Find(PlayerController));
				}
				else
				{
					Widget->SetText(0);
				}
			}
		}
		
		FInputModeGameAndUI InputMode;
		InputMode.SetWidgetToFocus(RollWidget->TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetHideCursorDuringCapture(true);
		PlayerController->SetInputMode(InputMode);
		PlayerController->SetShowMouseCursor(true);
	}
}

void ADHRollActor::EndInteract_Implementation(APlayerController* PlayerController)
{
	if (RollWidget)
	{
		if (UERNInteractableWidget* InteractableWidget = Cast<UERNInteractableWidget>(RollWidget))
		{
			InteractableWidget->BP_PlayCloseAnimation();
		}
	}
	
	if (Prompt)
	{
		Prompt->SetVisibility(false);
	}
}

void ADHRollActor::InitRoll()
{
	PlayerControllersRewordGrade.Empty();
	PlayerControllersOverFlag.Empty();
	PlayerControllersRewordFlag.Empty();
}

void ADHRollActor::Roll(APlayerController* PlayerController)
{
	if (PlayerControllersRewordFlag.Contains(PlayerController) || PlayerControllersOverFlag.Contains(PlayerController) )
	{
		return;
	}
	
	const AERNCharacterBase* Character = Cast<AERNCharacterBase>(PlayerController->GetCharacter());
	if (!Character)
	{
		return;
	}
	UERNAttributeSet* Attributes = Character->GetAttributeSet();
	if (!Attributes)
	{
		return;
	}
	
	const int32 Grade = PlayerControllersRewordGrade.FindOrAdd(PlayerController, 0);
	const int32 Value = FMath::RandRange(0, 100);
	if (Grade == 0)
	{
		if (Value >= 30)
		{
			if (UDHRollWidget* Widget = Cast<UDHRollWidget>(RollWidget))
			{
				Widget->SetText(1);
			}
			PlayerControllersRewordGrade[PlayerController] = 1;
		}
		else
		{
			if (UDHRollWidget* Widget = Cast<UDHRollWidget>(RollWidget))
			{
				Widget->SetText(-1);
				Widget->SetVisibilityRollButton(false);
				Widget->SetVisibilityRewardButton(false);
			}
			PlayerControllersOverFlag.Add(PlayerController, 0);
			const float NewGold = FMath::Max(Attributes->GetGold() - 5000.0f, 0.0f);
			Attributes->SetGold(NewGold);
		}
	}
	else if (Grade == 1)
	{
		if (Value >= 50)
		{
			if (UDHRollWidget* Widget = Cast<UDHRollWidget>(RollWidget))
			{
				Widget->SetText(2);
			}
			PlayerControllersRewordGrade[PlayerController] = 2;
		}
		else
		{
			if (UDHRollWidget* Widget = Cast<UDHRollWidget>(RollWidget))
			{
				Widget->SetText(-2);
				Widget->SetVisibilityRollButton(false);
				Widget->SetVisibilityRewardButton(false);
			}
			PlayerControllersOverFlag.Add(PlayerController, 1);
			const float NewGold = FMath::Max(Attributes->GetGold() - 20000.0f, 0.0f);
			Attributes->SetGold(NewGold);
		}
	}
	else if (Grade == 2)
	{
		if (Value >= 75)
		{
			if (UDHRollWidget* Widget = Cast<UDHRollWidget>(RollWidget))
			{
				Widget->SetText(3);
				Widget->SetVisibilityRollButton(false);
			}
			PlayerControllersRewordGrade[PlayerController] = 3;
		}
		else
		{
			if (UDHRollWidget* Widget = Cast<UDHRollWidget>(RollWidget))
			{
				Widget->SetText(-3);
				Widget->SetVisibilityRollButton(false);
				Widget->SetVisibilityRewardButton(false);
			}
			PlayerControllersOverFlag.Add(PlayerController, 2);
			Attributes->SetGold(0.0f);
		}
	}
}

void ADHRollActor::Reward(APlayerController* PlayerController)
{
	if (PlayerControllersRewordFlag.Contains(PlayerController))
	{
		return;
	}
	
	PlayerControllersRewordFlag.Add(PlayerController);
	
	if (!PlayerControllersRewordGrade.Contains(PlayerController))
	{
		return;
	}
	
	const AERNCharacterBase* Character = Cast<AERNCharacterBase>(PlayerController->GetCharacter());
	if (!Character)
	{
		return;
	}
	UERNAttributeSet* Attributes = Character->GetAttributeSet();
	if (!Attributes)
	{
		return;
	}
	
	const int32* Grade = PlayerControllersRewordGrade.Find(PlayerController);
	if (*Grade == 0)
	{
		if (UDHRollWidget* Widget = Cast<UDHRollWidget>(RollWidget))
		{
			Widget->SetText(4);
			Widget->SetVisibilityRollButton(false);
			Widget->SetVisibilityRewardButton(false);
		}
	}
	else if (*Grade == 1)
	{
		if (UDHRollWidget* Widget = Cast<UDHRollWidget>(RollWidget))
		{
			Widget->SetText(5);
			Widget->SetVisibilityRollButton(false);
			Widget->SetVisibilityRewardButton(false);
		}
		Attributes->SetGold(Attributes->GetGold() + 5000.0f);
	}
	else if (*Grade == 2)
	{
		if (UDHRollWidget* Widget = Cast<UDHRollWidget>(RollWidget))
		{
			Widget->SetText(6);
			Widget->SetVisibilityRollButton(false);
			Widget->SetVisibilityRewardButton(false);
		}
		Attributes->SetGold(Attributes->GetGold() + 20000.0f);
	}
	else if (*Grade == 3)
	{
		if (UDHRollWidget* Widget = Cast<UDHRollWidget>(RollWidget))
		{
			Widget->SetVisibilityRollButton(false);
			Widget->SetVisibilityRewardButton(false);
		}
		if (AERNPlayerController* ERNPC = Cast<AERNPlayerController>(PlayerController))
		{
			ERNPC->Server_RequestDHRollReward(this);
		}
	}
}

void ADHRollActor::SpawnRewardForPlayer(APlayerController* PlayerController)
{
	if (!HasAuthority() || !PlayerController)
	{
		return;
	}
	
	if (UItemManagerSubsystem* ItemManager = GetGameInstance()->GetSubsystem<UItemManagerSubsystem>())
	{
		if (const AERNPlayerState* PS = PlayerController->GetPlayerState<AERNPlayerState>())
		{
			FItemRuntimeState ItemRuntimeState;
			AActor* Item = nullptr;
				
			switch (PS->CharacterType)
			{
			case ECharacterType::Warrior:
				ItemRuntimeState.SetItemID(FName("SWORD_FROSTBANE_LEGENDARY"));
				ItemRuntimeState.SetQuantity(1);
				Item = ItemManager->SpawnItem(ItemRuntimeState, GetActorUpVector() * 250.0f, GetActorForwardVector().Rotation());
				break;
			case ECharacterType::Mage:
				ItemRuntimeState.SetItemID(FName("STAFF_SIVERSTAR_LEGENDARY"));
				ItemRuntimeState.SetQuantity(1);
				Item = ItemManager->SpawnItem(ItemRuntimeState, GetActorUpVector() * 250.0f, GetActorForwardVector().Rotation());
				break;
			case ECharacterType::Support:
				ItemRuntimeState.SetItemID(FName("POLEARM_HEAVENSHAKER_LEGENDARY"));
				ItemRuntimeState.SetQuantity(1);
				Item = ItemManager->SpawnItem(ItemRuntimeState, GetActorUpVector() * 250.0f, GetActorForwardVector().Rotation());
				break;
			default:
				break;
			}
				
			if (Item)
			{
				Item->SetOwner(Cast<AActor>(PlayerController));
				Item->bOnlyRelevantToOwner = true;
				if (AERNItemActor* ERNItem = Cast<AERNItemActor>(Item))
				{
					ERNItem->Launch(GetActorForwardVector());
					ERNItem->UpdateOwnerOnlyVisibility();
				}
			}
		}
	}
}

void ADHRollActor::Close()
{
	if (const AProjectERNCharacter* PlayerCharacter = Cast<AProjectERNCharacter>(GetWorld()->GetFirstPlayerController()->GetCharacter()))
	{
		if (AERNPlayerController* PC = Cast<AERNPlayerController>(PlayerCharacter->GetController()))
		{
			if (RollWidget)
			{
				RollWidget->RemoveFromParent();
				RollWidget = nullptr;
				
				PC->SetInputMode(FInputModeGameOnly());
				PC->SetShowMouseCursor(false);
				
				if (const ULocalPlayer* LocalPlayer = PC->GetLocalPlayer())
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
