// Fill out your copyright notice in the Description page of Project Settings.

#include "Inventory/Item/ERNItemActor.h"

#include "Character/Player/ERNPlayerController.h"
#include "Character/Player/ProjectERNCharacter.h"
#include "Components/SphereComponent.h"
#include "Data/EquipableItemDataAsset.h"
#include "Inventory/Components/ERNInventoryComponent.h"
#include "Manager/ItemManagerSubsystem.h"
#include "Net/UnrealNetwork.h"

// Sets default values
AERNItemActor::AERNItemActor()
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
	StaticMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
	SkeletalMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMesh"));
	SkeletalMesh->SetupAttachment(GetRootComponent());
	SkeletalMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AERNItemActor::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(AERNItemActor, ItemRuntimeState);
}

void AERNItemActor::BeginPlay()
{
	Super::BeginPlay();
	
	// Collision Overlap Binding
	Collision->OnComponentBeginOverlap.AddDynamic(this, &AERNItemActor::OnSphereBeginOverlap);
	Collision->OnComponentEndOverlap.AddDynamic(this, &AERNItemActor::OnSphereEndOverlap);
}

void AERNItemActor::Interact_Implementation(APlayerController* PlayerController)
{
	if (!HasAuthority())
	{
		return;
	}
	if (!PlayerController)
	{
		return;
	}
	const AProjectERNCharacter* Player = Cast<AProjectERNCharacter>(PlayerController->GetCharacter());
	if (!Player)
	{
		return;
	}
	
	// Inventory Component에 아이템 추가 요청
	if (UERNInventoryComponent* InventoryComponent = Player->GetInventoryComponent())
	{
		InventoryComponent->Server_AddItem(this);
	}
}

bool AERNItemActor::CanInteract_Implementation() const
{
	return true;
}

FText AERNItemActor::GetInteractionText_Implementation() const
{
	return FText::FromString(TEXT("E키를 눌러 아이템 습득"));
}

EInteractionExecutionPolicy AERNItemActor::GetInteractionExecutionPolicy_Implementation() const
{
	return EInteractionExecutionPolicy::ServerAuthority;
}

void AERNItemActor::InitializeRuntimeState(const FName ItemID, const int32 Quantity)
{
	// Item Runtime State Initialization
	ItemRuntimeState.ItemID = ItemID;
	ItemRuntimeState.Quantity = Quantity;

	// ItemRuntimeState 기반 DataAsset 비동기 로드
	LoadItemDataAssetFromRuntimeStateAsync();
}

void AERNItemActor::ApplyItemDataAsset(const UItemDataAssetBase* DataAsset)
{
	if (!IsValid(DataAsset))
	{
		return;
	}
	
	// Apply Mesh
	SetMesh(DataAsset);
}

UItemManagerSubsystem* AERNItemActor::GetItemManager() const
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

void AERNItemActor::SetMesh(const UItemDataAssetBase* DA) const
{
	if (!IsValid(DA))
	{
		return;
	}
	
	if (DA->StaticMesh)
	{
		StaticMesh->SetStaticMesh(DA->StaticMesh.Get());
	}
	else if (DA->SkeletalMesh)
	{
		SkeletalMesh->SetSkeletalMesh(DA->SkeletalMesh.Get());
	}
}

void AERNItemActor::LoadItemDataAssetFromRuntimeStateAsync()
{
	if (GetNetMode() == ENetMode::NM_DedicatedServer)
	{
		return;
	}
	// ItemRuntimeState 유효성 검사
	if (!ItemRuntimeState.IsValid())
	{
		return;
	}
	
	if (UItemManagerSubsystem* ItemManager = GetItemManager())
	{
		TWeakObjectPtr<AERNItemActor> WeakThis(this);
		// ItemDataAsset 비동기 로드
		ItemManager->PreloadItemDataAssetAsync(ItemRuntimeState.ItemID, EItemAssetLoadFlags::Gameplay, 
			FOnItemDataAssetLoaded::CreateLambda([WeakThis](const UItemDataAssetBase* LoadedDataAsset)
			{
				if (!WeakThis.IsValid() || !LoadedDataAsset)
				{
					return;
				}
				
				// 비동기 로드 완료 후 ItemDataAsset의 데이터 적용
				WeakThis->ApplyItemDataAsset(LoadedDataAsset);
			})
		);
	}
}

void AERNItemActor::OnRep_ItemRuntimeState()
{
	// ItemRuntimeState 기반 DataAsset 비동기 로드
	LoadItemDataAssetFromRuntimeStateAsync();
}

void AERNItemActor::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
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

void AERNItemActor::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
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
