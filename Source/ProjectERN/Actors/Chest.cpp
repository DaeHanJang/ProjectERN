// Fill out your copyright notice in the Description page of Project Settings.

#include "Actors/Chest.h"

#include "NiagaraFunctionLibrary.h"
#include "Character/Player/ERNPlayerController.h"
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
	Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Collision->SetCollisionResponseToAllChannels(ECR_Ignore);
	Collision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	
	// Mesh
	StaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
	StaticMesh->SetupAttachment(GetRootComponent());
	StaticMesh->SetCollisionProfileName(TEXT("BlockAll"));
}

// Called when the game starts or when spawned
void AChest::BeginPlay()
{
	Super::BeginPlay();
	
	// Collision Overlap Binding
	Collision->OnComponentBeginOverlap.AddDynamic(this, &AChest::OnSphereBeginOverlap);
	Collision->OnComponentEndOverlap.AddDynamic(this, &AChest::OnSphereEndOverlap);
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
	if (!StaticMesh)
	{
		return;
	}
	
	DynamicMaterial0 = StaticMesh->CreateAndSetMaterialInstanceDynamic(0);
	DynamicMaterial1 = StaticMesh->CreateAndSetMaterialInstanceDynamic(1);
	if (!DynamicMaterial0 || !DynamicMaterial1)
	{
		return;
	}
	
	bDestroyed = true;
	RemainingDissolveTime = DissolveTime;
	DynamicMaterial0->SetScalarParameterValue(TEXT("Dissolve"), 1.0f);
	DynamicMaterial1->SetScalarParameterValue(TEXT("Dissolve"), 1.0f);
	
	if (InteractSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, InteractSound, GetActorLocation());
	}
	
	if (InteractEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, InteractEffect, GetActorLocation());
	}
	
	Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	StaticMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
	if (!GetWorldTimerManager().IsTimerActive(DissolveTimerHandle))
	{
		GetWorldTimerManager().SetTimer(DissolveTimerHandle, this, &AChest::UpdateDissolve, DissolveRate, true);
	}
}

void AChest::UpdateDissolve()
{
	RemainingDissolveTime -= DissolveRate;
	
	const float Opacity = RemainingDissolveTime / DissolveTime;
	DynamicMaterial0->SetScalarParameterValue(TEXT("Dissolve"), Opacity);
	DynamicMaterial1->SetScalarParameterValue(TEXT("Dissolve"), Opacity);
	
	if (RemainingDissolveTime <= 0.0f)
	{
		GetWorldTimerManager().ClearTimer(DissolveTimerHandle);
		
		if (HasAuthority())
		{
			Destroy();
		}
	}
}

void AChest::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                  UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (const APawn* Pawn = Cast<APawn>(OtherActor))
	{
		if (AERNPlayerController* PC = Cast<AERNPlayerController>(Pawn->GetController()))
		{
			PC->SetCurrentInteractable(this);
		}
	}
}

void AChest::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (const APawn* Pawn = Cast<APawn>(OtherActor))
	{
		if (AERNPlayerController* PC = Cast<AERNPlayerController>(Pawn->GetController()))
		{
			PC->ClearCurrentInteractable();
		}
	}
}
