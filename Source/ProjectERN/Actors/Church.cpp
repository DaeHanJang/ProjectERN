// Fill out your copyright notice in the Description page of Project Settings.

#include "Actors/Church.h"

#include "NiagaraComponent.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"

// Sets default values
AChurch::AChurch()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	
	// Collision
	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	SetRootComponent(Collision);
	Collision->InitSphereRadius(150.0f);
	Collision->SetCollisionProfileName(TEXT("OverlapAll"));
	
	// Mesh
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(GetRootComponent());
	Mesh->SetCollisionProfileName(TEXT("BlockAll"));
	
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
	
}

bool AChurch::CanInteract_Implementation() const
{
	return !IsActorBeingDestroyed();
}

void AChurch::ActivateInteract_Implementation() const
{
	if (PromptComponent)
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
