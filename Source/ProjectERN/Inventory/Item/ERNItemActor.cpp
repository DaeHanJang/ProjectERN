// Fill out your copyright notice in the Description page of Project Settings.

#include "Inventory/Item/ERNItemActor.h"

#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Character/Player/ProjectERNCharacter.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "Data/EquipableItemDataAsset.h"
#include "Inventory/Components/ERNInventoryComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Manager/ItemManagerSubsystem.h"
#include "Net/UnrealNetwork.h"

// Sets default values
AERNItemActor::AERNItemActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	
	// Collision
	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	SetRootComponent(Collision);
	Collision->InitSphereRadius(150.0f);
	Collision->SetCollisionProfileName(TEXT("OverlapAll"));
	
	// Mesh
	StaticMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StaticMesh"));
	StaticMesh->SetupAttachment(GetRootComponent());
	StaticMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
	SkeletalMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMesh"));
	SkeletalMesh->SetupAttachment(GetRootComponent());
	SkeletalMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
	// Effect
	EffectComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("EffectComponent"));
	EffectComponent->SetupAttachment(GetRootComponent());
	
	// Prompt
	PromptComponent = CreateDefaultSubobject<UWidgetComponent>(TEXT("PromptComponent"));
	PromptComponent->SetupAttachment(GetRootComponent());
	PromptComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 100.f));
	static ConstructorHelpers::FClassFinder<UUserWidget> PromptClass(TEXT("/Game/Blueprint/Widget/WBP_InteractE"));
	if (PromptClass.Succeeded())
	{
		PromptComponent->SetWidgetClass(PromptClass.Class);
	}
	PromptComponent->SetWidgetSpace(EWidgetSpace::Screen);
	PromptComponent->SetVisibility(false);
	
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> CommonVFX(TEXT("/Game/Assets/VFX/Item/NS_CommonDropItem.NS_CommonDropItem"));
	if (CommonVFX.Succeeded())
	{
		CommonEffect = CommonVFX.Object;
	}
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> UncommonVFX(TEXT("/Game/Assets/VFX/Item/NS_UncommonDropItem.NS_UncommonDropItem"));
	if (UncommonVFX.Succeeded())
	{
		UncommonEffect = UncommonVFX.Object;
	}
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> RareVFX(TEXT("/Game/Assets/VFX/Item/NS_RareDropItem.NS_RareDropItem"));
	if (RareVFX.Succeeded())
	{
		RareEffect = RareVFX.Object;
	}
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> LegendaryVFX(TEXT("/Game/Assets/VFX/Item/NS_LegendaryDropItem.NS_LegendaryDropItem"));
	if (LegendaryVFX.Succeeded())
	{
		LegendaryEffect = LegendaryVFX.Object;
	}
}

void AERNItemActor::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(AERNItemActor, ItemRuntimeState);
}

void AERNItemActor::Interact_Implementation(APlayerController* PlayerController)
{
	if (!HasAuthority() || !PlayerController)
	{
		return;
	}
	if (!CanBeInteractedBy(PlayerController))
	{
		return;
	}
	AProjectERNCharacter* Player = Cast<AProjectERNCharacter>(PlayerController->GetCharacter());
	if (!Player)
	{
		return;
	}
	
	// Inventory Component에 아이템 추가 요청
	if (UERNInventoryComponent* InventoryComponent = Player->GetInventoryComponent())
	{
		InventoryComponent->Server_AddItem(this);
	}
	
	if (PickupSound)
	{
		Player->Multicast_PlayEffectAndSound(nullptr, FVector::ZeroVector, PickupSound, GetActorLocation());
	}
}

bool AERNItemActor::CanInteract_Implementation() const
{
	return !IsActorBeingDestroyed();
}

void AERNItemActor::ActivateInteract_Implementation() const
{
	if (!GetWorld())
	{
		return;
	}
	
	if (const APlayerController* PC = GetWorld()->GetFirstPlayerController())
	{
		if (!CanBeInteractedBy(PC))
		{
			return;
		}
	}
	
	if (PromptComponent)
	{
		PromptComponent->SetVisibility(true);
	}
}

void AERNItemActor::EndInteract_Implementation(APlayerController* PlayerController)
{
	if (PromptComponent)
	{
		PromptComponent->SetVisibility(false);
	}
}

FText AERNItemActor::GetInteractionText_Implementation() const
{
	return FText::FromString(TEXT("E키를 눌러 아이템 습득"));
}

EInteractionExecutionPolicy AERNItemActor::GetInteractionExecutionPolicy_Implementation() const
{
	return EInteractionExecutionPolicy::ServerAuthority;
}

void AERNItemActor::InitializeRuntimeState(const FItemRuntimeState& InItemRuntimeState)
{
	ItemRuntimeState = InItemRuntimeState;

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
	// Apply Effect
	SetEffect();
	// Apply Sound
	SetSound(DataAsset);
}

bool AERNItemActor::CanBeInteractedBy(const APlayerController* PC) const
{
	return !GetOwner() || GetOwner() == PC;
}

void AERNItemActor::UpdateOwnerOnlyVisibility() const
{
	const bool bOwnerOnly = GetOwner() != nullptr;
	if (!bOwnerOnly)
	{
		return;
	}
	
	const APlayerController* PC = GetWorld()->GetFirstPlayerController();
	const bool bVisibleToLocalPlayer = CanBeInteractedBy(PC);
	
	if (StaticMesh)
	{
		StaticMesh->SetVisibility(bVisibleToLocalPlayer, true);
	}
	if (SkeletalMesh)
	{
		SkeletalMesh->SetVisibility(bVisibleToLocalPlayer, true);
	}
	if (EffectComponent)
	{
		EffectComponent->SetVisibility(bVisibleToLocalPlayer, true);
	}
	if (PromptComponent)
	{
		PromptComponent->SetVisibility(false);
	}
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
		StaticMesh->SetRelativeRotation(DA->Rotator);
		StaticMesh->SetRelativeScale3D(DA->Scale);
	}
	else if (DA->SkeletalMesh)
	{
		SkeletalMesh->SetSkeletalMesh(DA->SkeletalMesh.Get());
		SkeletalMesh->SetRelativeRotation(DA->Rotator);
		SkeletalMesh->SetRelativeScale3D(DA->Scale);
	}
}

void AERNItemActor::SetEffect() const
{
	if (const UItemManagerSubsystem* ItemManager = GetItemManager())
	{
		if (const FERNItemTable* Row = ItemManager->FindItemRow(ItemRuntimeState.GetItemID()))
		{
			switch (Row->Grade)
			{
			case EItemGrade::Common:
				if (CommonEffect)
				{
					EffectComponent->SetAsset(CommonEffect);
				}
				break;
			case EItemGrade::Uncommon:
				if (UncommonEffect)
				{
					EffectComponent->SetAsset(UncommonEffect);
				}
				break;
			case EItemGrade::Rare:
				if (RareEffect)
				{
					EffectComponent->SetAsset(RareEffect);
				}
				break;
			case EItemGrade::Legendary:
				if (LegendaryEffect)
				{
					EffectComponent->SetAsset(LegendaryEffect);
				}
				break;
			default:
				if (CommonEffect)
				{
					EffectComponent->SetAsset(CommonEffect);
				}
				break;
			}
		}
	}
}

void AERNItemActor::SetSound(const UItemDataAssetBase* DA)
{
	if (!IsValid(DA))
	{
		return;
	}
	
	if (DA->Sound)
	{
		PickupSound = DA->Sound.Get();
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
		ItemManager->PreloadItemDataAssetAsync(ItemRuntimeState.GetItemID(), EItemAssetLoadFlags::Gameplay, 
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
