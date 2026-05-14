// Copyright Epic Games, Inc. All Rights Reserved.

#include "Inventory/Components/ERNEquipmentComponent.h"

#include "ERNInventoryComponent.h"
#include "Abilities/GameplayAbility.h"
#include "Character/Player/ProjectERNCharacter.h"
#include "Combat/Consumable/ERNConsumableBase.h"
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
	DOREPLIFETIME(UERNEquipmentComponent, CurrentConsumable);
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

void UERNEquipmentComponent::Server_EquipWeapon_Implementation(FName ItemID)
{
	// 기존 무기 제거
	if (CurrentWeapon)
	{
		CurrentWeapon->Destroy();
		CurrentWeapon = nullptr;
	}
	
	UItemManagerSubsystem* ItemManager = GetItemManagerSubsystem();
	if (!ItemManager)
	{
		return;
	}
	// 매개변수 유효성 검증
	if (!ItemManager->ItemValid(ItemID))
	{
		return;
	}
	
	// 장착할 아이템 데이터 애셋 로드
	const UEquipableItemDataAsset* DA = Cast<UEquipableItemDataAsset>(ItemManager->LoadItemDataAssetSync(ItemID, EItemAssetLoadFlags::Gameplay));
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
	
	if (!GetOwner())
	{
		return;
	}
	AERNCharacterBase* Character = Cast<AERNCharacterBase>(GetOwner());
	if (!Character || !Character->GetMesh())
	{
		return;
	}
	
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = GetOwner();
	// 장착할 무기 스폰
	AERNWeaponBase* NewWeapon = GetWorld()->SpawnActor<AERNWeaponBase>(WeaponClass, SpawnParams);
	if (!NewWeapon)
	{
		return;
	}
	
	// 무기 스킬 장착
	if (TSubclassOf<UGameplayAbility> WeaponSkillClass = DA->EquipableAbility.Get())
	{
		Character->SetWeaponAbility(WeaponSkillClass);
	}
	
	// 스폰된 아이템 런타임 값 설정
	FItemRuntimeState NewWeaponRuntimeState;
	NewWeaponRuntimeState.SetItemID(ItemID);
	NewWeaponRuntimeState.SetQuantity(1);
	NewWeapon->Init(NewWeaponRuntimeState, DA);
	
	// 장착할 아이템 부착
	NewWeapon->AttachToComponent(Character->GetMesh(), FAttachmentTransformRules::SnapToTargetIncludingScale, FName("WeaponSocket"));
	
	// 스폰된 아이템으로 변경
	CurrentWeapon = NewWeapon;
	
	// UI 갱신을 위한 슬롯 데이터 갱신 및 리슨 서버용 브로드캐스트
	FInventoryItemEntry Entry;
	Entry.SetItemRuntimeState(NewWeaponRuntimeState);
	EquipableSlot = Entry;
	OnEquipmentSlotChanged.Broadcast(EquipableSlot);
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
	if (!ItemManager)
	{
		return;
	}
	FInventoryItemEntry ItemEntry = Inventory.GetItems()[SlotIndex];
	// 인벤토리에서 장착할 아이템 검증
	if (!ItemEntry.IsValid() || !ItemManager->ItemValid(ItemEntry.GetItemID()))
	{
		return;
	}
	
	// 장착할 아이템 테이블 정보 가져오기
	const FERNItemTable* ItemRow = ItemManager->FindItemRow(ItemEntry.GetItemID());
	
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
		
		// GetOwner(), Character의 null체크는 GetInventoryComponent()에서 진행
		AERNCharacterBase* Character = Cast<AERNCharacterBase>(GetOwner());
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

		// 무기 스킬 장착
		if (TSubclassOf<UGameplayAbility> WeaponSkillClass = DA->EquipableAbility.Get())
		{
			Character->SetWeaponAbility(WeaponSkillClass);
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
		
		UE_LOG(LogTemp, Warning, TEXT("OldWeapon: %s"), *InventoryReplacementState.GetItemID().ToString());
		UE_LOG(LogTemp, Warning, TEXT("NewWeapon: %s"), *NewWeaponRuntimeState.GetItemID().ToString());
	}
	// 소포품인 경우
	else if (ItemRow->ItemType == EItemType::Consumable)
	{
		// 장착할 아이템 런타임 상태 값
		const FItemRuntimeState NewConsumableRuntimeState = ItemEntry.GetItemRuntimeState();
				
		// 다른 종류의 소모품일 경우 교환
		if (ItemEntry.GetItemID() != CurrentConsumable.GetItemID())
		{		
			// 장착할 아이템의 데이터 애셋 로드
			const UConsumableItemDataAsset* DA = Cast<UConsumableItemDataAsset>(ItemManager->LoadItemDataAssetSync(ItemEntry.GetItemID(), EItemAssetLoadFlags::Gameplay));
			if (!DA)
			{
				return;
			}
			
			// GetOwner(), Character의 null체크는 GetInventoryComponent()에서 진행
			AERNCharacterBase* Character = Cast<AERNCharacterBase>(GetOwner());
			if (!Character || !Character->GetMesh())
			{
				return;
			}
			
			// 장착할 아이템 런타임 값으로 변경
			CurrentConsumable = NewConsumableRuntimeState;
			
			// 소모품 사용 효과 장착
			if (TSubclassOf<UGameplayAbility> ConsumableAbility = DA->ConsumableAbility.Get())
			{
				Character->SetConsumableAbility(ConsumableAbility);
			}
		
			// 장착했던 아이템의 런타임 상태 값
			FItemRuntimeState InventoryReplacementState;
			// 장착했던 아이템의 런타임 상태
			if (ConsumableSlot.IsValid())
			{
				InventoryReplacementState = ConsumableSlot.GetItemRuntimeState();
			}
			
			ConsumableSlot = ItemEntry;
			
			// 장착했던 아이템의 런타임 상태 인벤토리 슬롯에 갱신
			Inventory.ChangeItem(SlotIndex, InventoryReplacementState);
			
			UE_LOG(LogTemp, Warning, TEXT("OldConsumable: %s, Quantity: %d"), *InventoryReplacementState.GetItemID().ToString(), InventoryReplacementState.GetQuantity());
		}
		// 같은 종류의 소모품일 경우 수량 계산
		else
		{
			const int32 MaxStackSize = ItemRow->MaxStackSize;
			const int32 AllQuantity = CurrentConsumable.GetQuantity() + ItemEntry.GetQuantity();
			if (AllQuantity <= MaxStackSize)
			{
				CurrentConsumable.SetQuantity(AllQuantity);
				ItemEntry.Init();
			}
			else
			{
				CurrentConsumable.SetQuantity(MaxStackSize);
				ItemEntry.SetQuantity(AllQuantity - MaxStackSize);
				if (ItemEntry.GetQuantity() <= 0)
				{
					ItemEntry.Init();
				}
			}
			
			FInventoryItemEntry CurrentConsumableEntry;
			CurrentConsumableEntry.SetItemRuntimeState(CurrentConsumable);			
			ConsumableSlot = CurrentConsumableEntry;
			
			Inventory.ChangeItem(SlotIndex, ItemEntry.GetItemRuntimeState());
		}
		
		InventoryComponent->OnInventorySlotChanged.Broadcast(Inventory.GetItems()[SlotIndex]);
				
		// UI 갱신을 위한 슬롯 데이터 갱신 및 리슨 서버용 브로드캐스트
		OnConsumableSlotChanged.Broadcast(ConsumableSlot);
		
		UE_LOG(LogTemp, Warning, TEXT("NewConsumable: %s, Quantity: %d"), *CurrentConsumable.GetItemID().ToString(), CurrentConsumable.GetQuantity());
	}
}

void UERNEquipmentComponent::Server_UnequipItem_Implementation(const int32 Quantity)
{
	// 매개변수 유효성 검증
	if (Quantity <= 0 || Quantity > CurrentConsumable.GetQuantity())
	{
		return;
	}
	
	// 월드에 생성할 드랍할 아이템의 런타임 상태는 CurrentConsumable을 복사 후 매개변수로 수량 설정
	FItemRuntimeState DropItemRuntimeState = CurrentConsumable;
	DropItemRuntimeState.SetQuantity(Quantity);
	
	// 장착 중인 소모품 수량 차감
	CurrentConsumable.AddQuantity(-Quantity);
	
	// 남은 수량이 0이면 슬롯 초기화, 아니면 ConsumableSlot 갱신
	if (CurrentConsumable.GetQuantity() <= 0)
	{
		CurrentConsumable.Init();
		ConsumableSlot.Init();
	}
	else
	{
		ConsumableSlot.SetItemRuntimeState(CurrentConsumable);
	}
	
	// UI 갱신을 위한 슬롯 데이터 갱신 및 리슨 서버용 브로드캐스트 
	OnConsumableSlotChanged.Broadcast(ConsumableSlot);
	
	// 소모품 슬롯에서 제거한 아이템 월드에 생성 
	const FVector& DropLocation = GetOwner()->GetActorLocation() + GetOwner()->GetActorForwardVector() * 50.0f;
	GetItemManagerSubsystem()->SpawnItem(DropItemRuntimeState, DropLocation, GetOwner()->GetActorRotation());
	
	UE_LOG(LogTemp, Warning, TEXT("DropConsumable: %s, Quantity: %d"), *DropItemRuntimeState.GetItemID().ToString(), DropItemRuntimeState.GetQuantity());
	UE_LOG(LogTemp, Warning, TEXT("CurrentConsumable: %s, Quantity: %d"), *CurrentConsumable.GetItemID().ToString(), CurrentConsumable.GetQuantity());
}
