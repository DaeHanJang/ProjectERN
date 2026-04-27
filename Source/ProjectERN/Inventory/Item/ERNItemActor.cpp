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
	
	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	SetRootComponent(Collision);
	Collision->InitSphereRadius(150.0f);
	Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Collision->SetCollisionResponseToAllChannels(ECR_Ignore);
	Collision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	
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
	
	Collision->OnComponentBeginOverlap.AddDynamic(this, &AERNItemActor::OnSphereBeginOverlap);
	Collision->OnComponentEndOverlap.AddDynamic(this, &AERNItemActor::OnSphereEndOverlap);
}

void AERNItemActor::Interact_Implementation(APlayerController* PlayerController)
{
	if (!PlayerController)
	{
		return;
	}
	
	const AProjectERNCharacter* Player = Cast<AProjectERNCharacter>(PlayerController->GetCharacter());
	if (!Player)
	{
		return;
	}
	
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

void AERNItemActor::InitRuntimeState(const FName ItemID, const int32 Quantity)
{
	ItemRuntimeState.ItemID = ItemID;
	ItemRuntimeState.Quantity = Quantity;
	RefreshFromItemRuntimeState();
}

void AERNItemActor::ApplyLoadedData(const UItemDataAssetBase* DataAsset)
{
	if (!IsValid(DataAsset))
	{
		return;
	}
	
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

void AERNItemActor::RefreshFromItemRuntimeState()
{
	if (!ItemRuntimeState.IsValid())
	{
		return;
	}
	
	if (UItemManagerSubsystem* ItemManager = GetItemManager())
	{
		TWeakObjectPtr<AERNItemActor> WeakThis(this);
		ItemManager->PreloadItemDataAssetAsync(ItemRuntimeState.ItemID, EItemAssetLoadFlags::All, 
			FOnItemDataAssetLoaded::CreateLambda([WeakThis](const UItemDataAssetBase* LoadedDataAsset)
			{
				if (!WeakThis.IsValid() || !LoadedDataAsset)
				{
					return;
				}
				
				WeakThis->ApplyLoadedData(LoadedDataAsset);
			})
		);
	}
}

void AERNItemActor::OnRep_ItemRuntimeState()
{
	RefreshFromItemRuntimeState();
}

void AERNItemActor::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                         UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// 플레이어가 범위 안에 들어오면 상호작용 UI 표시
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
	// 플레이어가 범위 밖으로 나가면 상호작용 UI 숨김
	if (const APawn* Pawn = Cast<APawn>(OtherActor))
	{
		if (AERNPlayerController* PC = Cast<AERNPlayerController>(Pawn->GetController()))
		{
			PC->ClearCurrentInteractable();
		}
	}
}
