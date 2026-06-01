// Fill out your copyright notice in the Description page of Project Settings.

#include "Actors/Church.h"

#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Character/Player/ERNPlayerController.h"
#include "Character/Player/ProjectERNCharacter.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
AChurch::AChurch()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	
	// Collision
	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	SetRootComponent(Collision);
	Collision->InitSphereRadius(250.0f);
	Collision->SetCollisionProfileName(TEXT("OverlapAll"));
	Collision->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel1, ECollisionResponse::ECR_Ignore);
	
	// Mesh
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(GetRootComponent());
	Mesh->SetCollisionProfileName(TEXT("BlockAll"));
	Mesh->SetCollisionResponseToChannel(ECollisionChannel::ECC_Visibility, ECollisionResponse::ECR_Overlap);
	
	// Effect
	EffectComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("EffectComponent"));
	EffectComponent->SetupAttachment(GetRootComponent());
	
	// Prompt
	PromptComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("PromptComponent"));
	PromptComponent->SetupAttachment(GetRootComponent());
	PromptComponent->SetWidgetSpace(EWidgetSpace::Screen);
	PromptComponent->SetVisibility(false);
}

void AChurch::Interact_Implementation(APlayerController* PlayerController)
{
	if (!HasAuthority())
	{
		return;
	}
	
	if (InteractedPlayerControllers.Contains(PlayerController))
	{
		return;
	}
	
	const AProjectERNCharacter* PlayerCharacter = Cast<AProjectERNCharacter>(PlayerController->GetPawn());
	if (!PlayerCharacter)
	{
		return;
	}
	
	PlayerCharacter->InteractionChurch();
	
	InteractedPlayerControllers.Add(PlayerController);
	if (AERNPlayerController* PC = Cast<AERNPlayerController>(PlayerController))
	{
		PC->Client_CompleteChurchInteraction(this);
	}
}

bool AChurch::CanInteract_Implementation() const
{	
	return !IsActorBeingDestroyed();
}

void AChurch::ActivateInteract_Implementation() const
{
	if (PromptComponent && PromptComponent->bHiddenInGame == false)
	{
		PromptComponent->SetVisibility(true);
	}
}

void AChurch::EndInteract_Implementation(APlayerController* PlayerController)
{
	if (PromptComponent)
	{
		PromptComponent->SetVisibility(false);
	}
}

EInteractionExecutionPolicy AChurch::GetInteractionExecutionPolicy_Implementation() const
{
	return EInteractionExecutionPolicy::ServerAuthority;
}

void AChurch::CompleteInteractionLocally(ACharacter* Character) const
{
	if (EffectComponent)
	{
		EffectComponent->DeactivateImmediate();
	}
	
	if (PromptComponent)
	{
		PromptComponent->SetHiddenInGame(true);
	}
	
	if (AERNCharacterBase* ERNCharacter = Cast<AERNCharacterBase>(Character))
	{
		if (!HasAuthority())
		{
			ERNCharacter->Server_RequestEffectAndSound(InteractionEffect, ERNCharacter->GetActorLocation(), InteractionSound, ERNCharacter->GetActorLocation());
		}
		else
		{
			ERNCharacter->Multicast_PlayEffectAndSound(InteractionEffect, ERNCharacter->GetActorLocation(), InteractionSound, ERNCharacter->GetActorLocation());
		}
	}
}
