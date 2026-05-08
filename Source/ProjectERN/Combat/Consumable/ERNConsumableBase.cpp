// Fill out your copyright notice in the Description page of Project Settings.

#include "Combat/Consumable/ERNConsumableBase.h"

#include "Abilities/GameplayAbility.h"
#include "Components/SphereComponent.h"
#include "Inventory/Item/Data/ConsumableItemDataAsset.h"

// Sets default values
AERNConsumableBase::AERNConsumableBase()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	SetReplicates(true);
	
	// Collision
	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	SetRootComponent(Collision);
	
	// Mesh
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(GetRootComponent());
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AERNConsumableBase::Init(const FItemRuntimeState& InRuntimeState, const UConsumableItemDataAsset* DA)
{
	if (!DA)
	{
		return;
	}
	
	if (!DA->ConsumableAbility.IsNull())
	{
		GA = DA->ConsumableAbility.Get();
	}
	RuntimeState.SetItemID(InRuntimeState.GetItemID());
	RuntimeState.SetQuantity(InRuntimeState.GetQuantity());
}
