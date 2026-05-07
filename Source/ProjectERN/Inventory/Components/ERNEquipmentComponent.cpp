// Copyright Epic Games, Inc. All Rights Reserved.

#include "Inventory/Components/ERNEquipmentComponent.h"

#include "ERNInventoryComponent.h"
#include "Character/Player/ProjectERNCharacter.h"
#include "Combat/Weapons/ERNWeaponBase.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Character.h"
#include "Components/SkeletalMeshComponent.h"
#include "Inventory/Item/Data/ConsumableItemDataAsset.h"
#include "Inventory/Item/Data/EquipableItemDataAsset.h"
#include "Inventory/Item/Manager/ItemManagerSubsystem.h"

UERNEquipmentComponent::UERNEquipmentComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UERNEquipmentComponent::BeginPlay()
{
	Super::BeginPlay();
}

UItemManagerSubsystem* UERNEquipmentComponent::GetItemManagerSubsystem() const
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

UERNInventoryComponent* UERNEquipmentComponent::GetInventoryComponent() const
{
	if (const AActor* Owner = GetOwner())
	{
		if (const AProjectERNCharacter* PlayerCharacter = Cast<AProjectERNCharacter>(Owner))
		{
			if (UERNInventoryComponent* InventoryComponent = PlayerCharacter->GetInventoryComponent())
			{
				return InventoryComponent;
			}
		}
	}
	
	return nullptr;
}

void UERNEquipmentComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UERNEquipmentComponent, CurrentWeapon);
	DOREPLIFETIME(UERNEquipmentComponent, EquipableSlot);
	// TODO: 현재 장착된 소모품 변수 추가
	//DOREPLIFETIME(UERNEquipmentComponent, CurrentConsumable);
	DOREPLIFETIME(UERNEquipmentComponent, ConsumableSlot);
}

void UERNEquipmentComponent::OnRep_EquipableSlot()
{
	OnEquipmentSlotChanged.Broadcast(EquipableSlot);
}

void UERNEquipmentComponent::OnRep_ConsumableSlot()
{
	OnConsumableSlotChanged.Broadcast(ConsumableSlot);
}

void UERNEquipmentComponent::Server_EquipWeapon_Implementation(TSubclassOf<AERNWeaponBase> WeaponClass)
{
	// 기존 무기 제거
	if (CurrentWeapon)
	{
		CurrentWeapon->Destroy();
		CurrentWeapon = nullptr;
	}

	// 새 무기 스폰
	if (WeaponClass)
	{
		AActor* Owner = GetOwner();
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = Owner;

		CurrentWeapon = GetWorld()->SpawnActor<AERNWeaponBase>(WeaponClass, SpawnParams);

		// 캐릭터 메시에 부착
		if (CurrentWeapon && Owner)
		{
			ACharacter* Character = Cast<ACharacter>(Owner);
			if (Character && Character->GetMesh())
			{
				CurrentWeapon->AttachToComponent(
					Character->GetMesh(),
					FAttachmentTransformRules::SnapToTargetIncludingScale,
					FName("WeaponSocket")
				);

				UE_LOG(LogTemp, Log, TEXT("Weapon equipped: %s"), *WeaponClass->GetName());
			}
		}
	}
}

void UERNEquipmentComponent::Server_UnequipWeapon_Implementation()
{
	if (CurrentWeapon)
	{
		CurrentWeapon->Destroy();
		CurrentWeapon = nullptr;
		UE_LOG(LogTemp, Log, TEXT("Weapon unequipped"));
	}
}

void UERNEquipmentComponent::Server_EquipItem_Implementation(const int32 SlotIndex)
{
	UERNInventoryComponent* InventoryComponent = GetInventoryComponent();
	if (!InventoryComponent)
	{
		return;
	}
	FInventoryList& Inventory = InventoryComponent->GetInventory();
	// 매개변수 유효성 검사
	if (!Inventory.GetItems().IsValidIndex(SlotIndex))
	{
		return;
	}
	
	UItemManagerSubsystem* ItemManager = GetItemManagerSubsystem();
	// 현재 장착중인 아이템 검증
	if (!ItemManager)
	{
		return;
	}
	
	const FInventoryItemEntry ItemEntry = Inventory.GetItems()[SlotIndex];
	// 인벤토리에서 장착할 아이템 검증
	if (!ItemEntry.IsValid() || !ItemManager->ItemValid(ItemEntry.GetItemID()))
	{
		return;
	}
	
	// 장착할 아이템 테이블 정보 가져오기
	const FERNItemTable* ItemRow = ItemManager->FindItemRow(ItemEntry.GetItemID());
	if (!ItemRow)
	{
		return;
	}
	
	// 장비인 경우
	if (ItemRow->ItemType == EItemType::Equipable)
	{
		// 장착할 아이템의 데이터 애셋 로드
		const UEquipableItemDataAsset* DA = Cast<UEquipableItemDataAsset>(ItemManager->LoadItemDataAssetSync(ItemEntry.GetItemID(), EItemAssetLoadFlags::Gameplay));
		if (!DA || DA->EquipableClass.IsNull())
		{
			return;
		}
		
		// 로드한 데이터 애셋에 있는 무기 클래스
		TSubclassOf<AERNWeaponBase> WeaponClass = DA->EquipableClass.Get();
		if (!WeaponClass)
		{
			return;
		}
		
		const ACharacter* Character = Cast<ACharacter>(GetOwner());
		if (!Character || !Character->GetMesh())
		{
			return;
		}
		
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = GetOwner();
		// 장착할 아이템 스폰
		AERNWeaponBase* NewWeapon = GetWorld()->SpawnActor<AERNWeaponBase>(WeaponClass, SpawnParams);
		if (!NewWeapon)
		{
			return;
		}
		
		// 스폰된 아이템 런타임 값 설정
		const FItemRuntimeState NewWeaponRuntimeState = ItemEntry.GetItemRuntimeState();
		NewWeapon->Init(NewWeaponRuntimeState, DA);
		
		// 장착할 아이템 부착
		NewWeapon->AttachToComponent(Character->GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, FName("WeaponSocket"));
		
		// 장착했던 아이템의 런타임 상태
		FItemRuntimeState InventoryReplacementState;
		if (EquipableSlot.IsValid())
		{
			InventoryReplacementState = EquipableSlot.GetItemRuntimeState();
		}
		
		// 현재 장착중인 무기 제거
		if (CurrentWeapon)
		{
			CurrentWeapon->Destroy();
		}
		// 스폰된 아이템으로 변경
		CurrentWeapon = NewWeapon;
		
		// 장착했던 아이템의 런타임 상태 인벤토리 슬롯에 갱신
		Inventory.ChangeItem(SlotIndex, InventoryReplacementState);
		InventoryComponent->OnInventorySlotChanged.Broadcast(Inventory.GetItems()[SlotIndex]);
		
		// UI 갱신을 위한 슬롯 데이터 갱신 및 리슨 서버용 브로드캐스트
		EquipableSlot = ItemEntry;
		OnEquipmentSlotChanged.Broadcast(EquipableSlot);
		
		UE_LOG(LogTemp, Log, TEXT("Weapon equipped: %s"), *DA->EquipableClass.Get()->GetName());
	}
	// TODO: 소모품 클래스를 만든 후 작성
	// 소포품인 경우
	else if (ItemRow->ItemType == EItemType::Consumable)
	{
		// 장착할 아이템의 데이터 애셋 로드
		//const UConsumableItemDataAsset* DA = Cast<UConsumableItemDataAsset>(ItemManager->LoadItemDataAssetSync(ItemEntry.GetItemID(), EItemAssetLoadFlags::Gameplay));
		//if (!DA || DA->ConsumableClass.IsNull())
		//{
		//	return;
		//}
		
		// 로드한 데이터 애셋에 있는 소모품 클래스
		//TSubclassOf<AERNConsumableBase> ConsumableClass = DA->ConsumableClass.Get();
		//if (!ConsumableClass)
		//{
		//	return;
		//}
		
		// 인벤토리에서 현재 장착한 아이템으로 데이터 교체
	
		// 현재 장착한 아이템 제거
		
		// 장착할 아이템 설정
		
		// UI 갱신을 위한 슬롯 데이터 갱신 및 리슨 서버용 브로드캐스트
		ConsumableSlot = ItemEntry;
		OnConsumableSlotChanged.Broadcast(ConsumableSlot);
		
		//UE_LOG(LogTemp, Log, TEXT("Consumable equipped: %s"), *DA->EquipableClass.Get()->GetName());
	}
}

void UERNEquipmentComponent::Server_UnequipItem_Implementation()
{
	// TODO: 소모품 클래스를 만든 후 작성
}
