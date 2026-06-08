// Fill out your copyright notice in the Description page of Project Settings.

#include "Actors/Chest.h"

#include "NiagaraComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "Inventory/Item/ERNItemActor.h"
#include "Inventory/Item/Data/ERNDropTable.h"
#include "Inventory/Item/Data/ERNItemRuntimeState.h"
#include "Inventory/Item/Manager/ItemManagerSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "World/ItemChestSpawner.h"

// Sets default values
AChest::AChest()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	// Collision
	Collision = CreateDefaultSubobject<UBoxComponent>(TEXT("Collision"));
	SetRootComponent(Collision);
	Collision->InitBoxExtent(FVector(40.0f, 80.0f, 40.0f));
	Collision->SetCollisionProfileName(TEXT("BlockAll"));
	
	// Mesh
	SkeletalMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMesh"));
	SkeletalMesh->SetupAttachment(GetRootComponent());
	SkeletalMesh->SetCollisionProfileName(TEXT("BlockAll"));
	SkeletalMesh->SetRelativeLocationAndRotation(FVector(0.0f, 0.0f, -40.0f), FRotator(0.0f, 90.0f, 0.0f));
	
	// InteractionCollision
	InteractionCollision = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionCollision"));
	InteractionCollision->SetupAttachment(GetRootComponent());
	InteractionCollision->InitSphereRadius(150.0f);
	InteractionCollision->SetCollisionProfileName(TEXT("OverlapAll"));
	InteractionCollision->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel1, ECollisionResponse::ECR_Ignore);
	
	// Effect
	EffectComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("EffectComponent"));
	EffectComponent->SetupAttachment(GetRootComponent());
	EffectComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 50.0f));
	
	// Prompt
	PromptComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("PromptComponent"));
	PromptComponent->SetupAttachment(GetRootComponent());
	PromptComponent->SetWidgetSpace(EWidgetSpace::Screen);
	PromptComponent->SetVisibility(false);
	PromptComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 100.0f));
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
			const FVector DropLocation = GetActorLocation() + FVector::UpVector * 50.0f;
			AActor* Item = ItemManager->SpawnItem(ItemRuntimeState, DropLocation, GetActorForwardVector().Rotation());
			if (AERNItemActor* ERNItem = Cast<AERNItemActor>(Item))
			{
				ERNItem->Launch(-GetActorForwardVector());
			}
		}
	}
	
	Dissolve();
	
	//상자 열림에 따른 상자 스포너에 대한 처리
	if (AItemChestSpawner* Spawner = Cast<AItemChestSpawner>(GetOwner()))
	{
		Spawner->HandleChestActivated();
	}
}

bool AChest::CanInteract_Implementation() const
{
	return !IsActorBeingDestroyed() && DropTable && DropTable->GetRowStruct() == FERNDropTable::StaticStruct() && !bOpened && !bDestroyed;
}

void AChest::ActivateInteract_Implementation() const
{
	if (PromptComponent && CanInteract_Implementation())
	{
		PromptComponent->SetVisibility(true);
	}
}

void AChest::EndInteract_Implementation(APlayerController* PlayerController)
{
	if (PromptComponent)
	{
		PromptComponent->SetVisibility(false);
	}
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
	
	if (PromptComponent)
	{
		PromptComponent->SetVisibility(false);
	}
	if (InteractAnimation)
	{
		SkeletalMesh->PlayAnimation(InteractAnimation, false);
	}
	if (InteractEffect)
	{
		EffectComponent->SetAsset(InteractEffect);
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
	InteractionCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
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
