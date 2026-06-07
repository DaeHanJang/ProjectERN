// Fill out your copyright notice in the Description page of Project Settings.


#include "ItemChestSpawner.h"

#include "ERNWorldManagerSubsystem.h"
#include "Actors/Chest.h"


// Sets default values
AItemChestSpawner::AItemChestSpawner()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
}

// Called when the game starts or when spawned
void AItemChestSpawner::BeginPlay()
{
	Super::BeginPlay();
	
	if (HasAuthority() == false)
	{
		return;
	}
	
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}
	
	UERNWorldManagerSubsystem* WorldManager = World->GetSubsystem<UERNWorldManagerSubsystem>();
	if (WorldManager == nullptr)
	{
		return;
	}
	
	if (ItemChestSpawnerID.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("ItemChestSpawnerID is Null"));
		return;
	}
	
	WorldManager->RegisterItemChestSpawner(ItemChestSpawnerID);
	
	// 이미 활성화 된 상자라면 생성하지 않음
	if (WorldManager->IsItemChestOpened(ItemChestSpawnerID) == true)
	{
		return;
	}
	
	SpawnChest();
}

void AItemChestSpawner::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (HasAuthority() == false)
	{
		return;
	}
	
	ClearSpawnedChest();
	
	Super::EndPlay(EndPlayReason);
}

void AItemChestSpawner::EnsureItemChestSpawnerID()
{
	if (ItemChestSpawnerID.IsNone() == false)
	{
		return;
	}
	
	//몹 스포너 ID 비어 있으면 할당. ItemSpawner + GUID
	Modify();
	
	const FString NewID = FString::Printf(TEXT("ItemChestSpawner_%s"), *FGuid::NewGuid().ToString(EGuidFormats::Short));
	ItemChestSpawnerID = FName(*NewID);
	
}

void AItemChestSpawner::SpawnChest()
{
	AChest* Chest = GetWorld()->SpawnActor<AChest>(ChestClass, GetActorTransform());
	
	if (Chest)
	{
		Chest->AttachToActor(this,FAttachmentTransformRules::KeepWorldTransform);
		Chest->SetOwner(this);
		Chest->SetDropTable(DropTable);
		SpawnedChest = Chest;
	}
}

void AItemChestSpawner::HandleChestActivated()
{
	if (HasAuthority() == false)
	{
		return;
	}
	
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}
	
	UERNWorldManagerSubsystem* WorldManager = World->GetSubsystem<UERNWorldManagerSubsystem>();
	if (WorldManager == nullptr)
	{
		return;
	}
	
	// 아이템 상자 활성화 처리
	WorldManager->SetItemChestActive(ItemChestSpawnerID, true);
}

void AItemChestSpawner::ClearSpawnedChest()
{
	if (SpawnedChest == nullptr)
	{
		return;
	}
	
	//처리중이 아니라면 직접 처리
	if (SpawnedChest->IsActorBeingDestroyed() == false)
	{
		SpawnedChest->Destroy();
	}
	
	SpawnedChest = nullptr;
}
