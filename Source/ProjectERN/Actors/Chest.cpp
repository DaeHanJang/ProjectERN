// Fill out your copyright notice in the Description page of Project Settings.

#include "Actors/Chest.h"

#include "NiagaraFunctionLibrary.h"
#include "Components/SphereComponent.h"
#include "Inventory/Item/Data/ERNDropTable.h"
#include "Inventory/Item/Data/ERNItemRuntimeState.h"
#include "Inventory/Item/Manager/ItemManagerSubsystem.h"
#include "Kismet/GameplayStatics.h"

// Sets default values
AChest::AChest()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	SetReplicates(true);

	// Collision
	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	SetRootComponent(Collision);
	Collision->InitSphereRadius(150.0f);
	Collision->SetCollisionProfileName(TEXT("OverlapAll"));
	
	// Mesh
	SkeletalMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("StaticMesh"));
	SkeletalMesh->SetupAttachment(GetRootComponent());
	SkeletalMesh->SetCollisionProfileName(TEXT("BlockAll"));
}

void AChest::Interact_Implementation(APlayerController* PlayerController)
{
	if (!HasAuthority())
	{
		return;
	}
	if (!PlayerController)
	{
		return;
	}
		
	bOpened = true;
	if (UItemManagerSubsystem* ItemManager = GetItemManager())
	{
		FItemRuntimeState ItemRuntimeState;
		if (ItemManager->RollItemFromDropTable(DropTable, ItemRuntimeState))
		{
			ItemManager->SpawnItem(ItemRuntimeState, GetActorLocation() + FVector(-150.0f, 0.0f, 50.0f), GetActorRotation());
		}
	}
	
	Dissolve();
}

bool AChest::CanInteract_Implementation() const
{
	return !IsActorBeingDestroyed() && DropTable && DropTable->GetRowStruct() == FERNDropTable::StaticStruct() && !bOpened;
}

FText AChest::GetInteractionText_Implementation() const
{
	return FText::FromString(TEXT("E키를 눌러 상자 열기"));
}

EInteractionExecutionPolicy AChest::GetInteractionExecutionPolicy_Implementation() const
{
	return EInteractionExecutionPolicy::ServerAuthority;
}

UItemManagerSubsystem* AChest::GetItemManager() const
{
	if (const UWorld* World = GetWorld())
	{
		if (const UGameInstance* GI = World->GetGameInstance())
		{
			return GI->GetSubsystem<UItemManagerSubsystem>();
		}
	}
	
	return nullptr;
}

void AChest::Dissolve_Implementation()
{
	if (bDestroyed)
	{
		return;
	}
	if (!SkeletalMesh)
	{
		return;
	}
	
	DynamicMaterial0 = SkeletalMesh->CreateAndSetMaterialInstanceDynamic(0);
	if (!DynamicMaterial0)
	{
		return;
	}
	
	bDestroyed = true;
	RemainingDissolveTime = DissolveTime;
	DynamicMaterial0->SetScalarParameterValue(TEXT("Dissolve"), 1.0f);
	
	if (InteractAnimation)
	{
		SkeletalMesh->PlayAnimation(InteractAnimation, false);
	}
	if (InteractEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, InteractEffect, GetActorLocation());
	}
	if (InteractSound0)
	{
		UGameplayStatics::PlaySoundAtLocation(this, InteractSound0, GetActorLocation());
	}
	if (InteractSound1)
	{
		UGameplayStatics::PlaySoundAtLocation(this, InteractSound1, GetActorLocation());
	}
	
	Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SkeletalMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
	if (!GetWorldTimerManager().IsTimerActive(DissolveTimerHandle))
	{
		GetWorldTimerManager().SetTimer(DissolveTimerHandle, this, &AChest::UpdateDissolve, DissolveRate, true, 1.0f);
	}
}

void AChest::UpdateDissolve()
{
	RemainingDissolveTime -= DissolveRate;
	
	const float Opacity = RemainingDissolveTime / DissolveTime;
	DynamicMaterial0->SetScalarParameterValue(TEXT("Dissolve"), Opacity);
	
	if (RemainingDissolveTime <= 0.0f)
	{
		GetWorldTimerManager().ClearTimer(DissolveTimerHandle);
		
		if (HasAuthority())
		{
			Destroy();
		}
	}
}
