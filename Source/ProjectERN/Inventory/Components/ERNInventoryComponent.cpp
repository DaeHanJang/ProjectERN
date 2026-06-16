// Copyright Epic Games, Inc. All Rights Reserved.

#include "Inventory/Components/ERNInventoryComponent.h"

#include "Character/ERNCharacterBase.h"
#include "Character/Player/ProjectERNCharacter.h"
#include "GAS/ERNAttributeSet.h"
#include "Inventory/Item/ERNItemActor.h"
#include "Inventory/Item/Manager/ItemManagerSubsystem.h"
#include "Net/UnrealNetwork.h"

UERNInventoryComponent::UERNInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	Inventory.SetOwner(this);
}

void UERNInventoryComponent::CopyInventoryFrom(const UERNInventoryComponent* Source)
{
	if (!Source)
	{
		return;
	}
	
	Inventory.CopyFrom(Source->GetInventory(), MaxSlotSize);
	
	for (const FInventoryItemEntry& Entry : Inventory.GetItems())
	{
		OnInventorySlotChanged.Broadcast(Entry);
	}
}

void UERNInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	// 스냅샷 복원이 먼저 일어났으면(아이템 존재) 덮어쓰지 않음
	if (Inventory.GetItems().Num() == 0)
	{
		Inventory.Init(MaxSlotSize);
	}
}

void UERNInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UERNInventoryComponent, Inventory);
}

UItemManagerSubsystem* UERNInventoryComponent::GetItemManager() const
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

void UERNInventoryComponent::Server_AddItem_Implementation(AERNItemActor* ItemActor)
{
	// 매개 변수 유효성 검사
	if (!ItemActor)
	{
		return;
	}
	
	FItemRuntimeState& ItemRuntimeState = ItemActor->GetItemRuntimeState();
	// ItemActor의 ItemRuntimeState가 유효하지 않을 때
	if (!ItemRuntimeState.IsValid())
	{
		return;
	}
	
	const UItemManagerSubsystem* ItemManager = GetItemManager();
	// 서버에서 ItemID 유효성 검사
	if (!ItemManager || !ItemManager->ItemValid(ItemRuntimeState.GetItemID()))
	{
		return;
	}
	
	TArray<FInventoryItemEntry> ChangedSlots;
	// Inventory에 넣기 요청
	const FERNItemTable* ItemTable = ItemManager->FindItemRow(ItemRuntimeState.GetItemID());
	if (!Inventory.AddItem(ItemRuntimeState, MaxSlotSize, ItemTable->MaxStackSize, ChangedSlots))
	{
		return;
	}
	
	if (ItemTable->ItemType == EItemType::Equipable)
	{
		ApplyItemAbility(ItemRuntimeState, ItemTable->Grade);
	}
	
	// 리슨 서버 인벤토리 변경 이벤트 발신 (UI 갱신)
	for (const FInventoryItemEntry& ChangedSlot : ChangedSlots)
	{
		OnInventorySlotChanged.Broadcast(ChangedSlot);
	}
	
	Inventory.LogInventory();
	
	if (ItemRuntimeState.GetQuantity() <= 0)
	{
		ItemActor->Destroy();
	}
	
	UE_LOG(LogTemp, Warning, TEXT("Item: %s, Quantity: %d, Ability: %d"), *ItemRuntimeState.GetItemID().ToString(), ItemRuntimeState.GetQuantity(), static_cast<int32>(ItemRuntimeState.GetItemAbility()));
}

void UERNInventoryComponent::Server_RemoveItem_Implementation(const int32 SlotIndex, const int32 Count)
{
	// 매개 변수 유효성 검사
	if (!Inventory.GetItems().IsValidIndex(SlotIndex))
	{
		return;
	}
	if (Count <= 0 || Count > GetItemQuantity(SlotIndex))
	{
		return;
	}
	
	// 리슨 서버 UI 갱신용 갱신된 인벤토리 슬롯 배열
	FInventoryItemEntry ChangedSlot;
	// 인벤토리에서 제거 후 월드에 생성할 아이템 액터의 런타임 상태
	FItemRuntimeState DropItemRuntimeState;
	// 인벤토리에서 아이템 제거
	Inventory.RemoveItem(SlotIndex, Count, DropItemRuntimeState, ChangedSlot);
	
	const UItemManagerSubsystem* ItemManager = GetItemManager();
	// 서버에서 ItemID 유효성 검사
	if (!ItemManager || !ItemManager->ItemValid(DropItemRuntimeState.GetItemID()))
	{
		return;
	}
	const FERNItemTable* ItemTable = ItemManager->FindItemRow(DropItemRuntimeState.GetItemID());
	if (ItemTable->ItemType == EItemType::Equipable)
	{
		RemoveItemAbility(DropItemRuntimeState, ItemTable->Grade);
		UE_LOG(LogTemp, Warning, TEXT("Item: %s, Quantity: %d, Ability: %d"), *DropItemRuntimeState.GetItemID().ToString(), DropItemRuntimeState.GetQuantity(), static_cast<int32>(DropItemRuntimeState.GetItemAbility()));
	}
	
	// 리슨 서버 전용 UI 갱신 이벤트
	OnInventorySlotChanged.Broadcast(ChangedSlot);
	
	Inventory.LogInventory();
	
	// 인벤토리에서 제거한 아이템 월드에 생성 
	const FVector& DropLocation = GetOwner()->GetActorLocation() + GetOwner()->GetActorForwardVector() * 50.0f;
	AActor* Item = GetItemManager()->SpawnItem(DropItemRuntimeState, DropLocation, GetOwner()->GetActorRotation());
	if (AERNItemActor* ERNItem = Cast<AERNItemActor>(Item))
	{
		ERNItem->Launch(GetOwner()->GetActorForwardVector());
	}
	
	UE_LOG(LogTemp, Warning, TEXT("DropItem: %s, Quantity: %d"), *DropItemRuntimeState.GetItemID().ToString(), DropItemRuntimeState.GetQuantity());
	UE_LOG(LogTemp, Warning, TEXT("CurrentItem: %s, Quantity: %d"), *Inventory.GetItems()[SlotIndex].GetItemID().ToString(), Inventory.GetItemQuantity(SlotIndex));
}

void UERNInventoryComponent::ApplyItemAbility(const FItemRuntimeState& ItemRuntimeState, EItemGrade Grade) const
{
	AERNCharacterBase* Character = Cast<AERNCharacterBase>(GetOwner());
	if (!Character)
	{
		return;
	}
	
	UERNAttributeSet* AttributeSet = Character->GetAttributeSet();
	if (!AttributeSet)
	{
		return;
	}
	
	const int32 Weight = static_cast<int32>(Grade) + 1;
	float Value, Value2;
	switch (ItemRuntimeState.GetItemAbility())
	{
	case EItemAbility::Health:
		Value = 20.0f * Weight;
		AttributeSet->SetMaxHealth(AttributeSet->GetMaxHealth() + Value);
		AttributeSet->SetHealth(AttributeSet->GetHealth() + Value);
		break;
	case EItemAbility::Attack:
		Value = 2.0f * Weight;
		AttributeSet->SetAttackPower(AttributeSet->GetAttackPower() + Value);
		break;
	case EItemAbility::HealthAndAttack:
		Value = 10.0f * Weight;
		Value2 = 1.0f * Weight;
		AttributeSet->SetMaxHealth(AttributeSet->GetMaxHealth() + Value);
		AttributeSet->SetHealth(AttributeSet->GetHealth() + Value);
		AttributeSet->SetAttackPower(AttributeSet->GetAttackPower() + Value2);
		break;
	case EItemAbility::Stamina:
		Value = 10.0f * Weight;
		AttributeSet->SetMaxStamina(AttributeSet->GetMaxStamina() + Value);
		AttributeSet->SetStamina(AttributeSet->GetStamina() + Value);
		break;
	case EItemAbility::Defence:
		Value = 1.0f * Weight;
		AttributeSet->SetDefense(AttributeSet->GetDefense() + Value);
		break;
	case EItemAbility::Gold:
		Value = 50.0f * Weight;
		Character->AddGoldWeight(Value);
		break;
	case EItemAbility::Drain:
		Value = 0.5f * Weight;
		if (AProjectERNCharacter* PlayerCharacter = Cast<AProjectERNCharacter>(Character))
		{
			PlayerCharacter->LifestealFraction += Value;
		}
		break;
	case EItemAbility::HealthCurse:
		Value = 50.0f * Weight;
		Value2 = 5.0f * Weight;
		AttributeSet->SetMaxHealth(AttributeSet->GetMaxHealth() - Value);
		if (AttributeSet->GetMaxHealth() < AttributeSet->GetHealth())
		{
			AttributeSet->SetHealth(AttributeSet->GetMaxHealth());
		}
		AttributeSet->SetAttackPower(AttributeSet->GetAttackPower() + Value2);
		break;
	case EItemAbility::AttackCurse:
		Value = 5.0f * Weight;
		Value2 = 1.0f * Weight;
		AttributeSet->SetAttackPower(AttributeSet->GetAttackPower() - Value);
		AttributeSet->SetDefense(AttributeSet->GetDefense() + Value2);
		AttributeSet->SetMaxHealth(AttributeSet->GetMaxHealth() + Value2 * 20.0f);
		AttributeSet->SetHealth(AttributeSet->GetHealth() + Value2 * 20.0f);
		break;
	default:
		break;
	}	
}

void UERNInventoryComponent::RemoveItemAbility(const FItemRuntimeState& ItemRuntimeState, EItemGrade Grade) const
{
	AERNCharacterBase* Character = Cast<AERNCharacterBase>(GetOwner());
	if (!Character)
	{
		return;
	}
	
	UERNAttributeSet* AttributeSet = Character->GetAttributeSet();
	if (!AttributeSet)
	{
		return;
	}
	
	const int32 Weight = static_cast<int32>(Grade) + 1;
	float Value, Value2;
	switch (ItemRuntimeState.GetItemAbility())
	{
	case EItemAbility::Health:
		Value = 20.0f * Weight;
		AttributeSet->SetMaxHealth(AttributeSet->GetMaxHealth() - Value);
		if (AttributeSet->GetMaxHealth() < AttributeSet->GetHealth())
		{
			AttributeSet->SetHealth(AttributeSet->GetMaxHealth());
		}
		break;
	case EItemAbility::Attack:
		Value = 2.0f * Weight;
		AttributeSet->SetAttackPower(AttributeSet->GetAttackPower() - Value);
		break;
	case EItemAbility::HealthAndAttack:
		Value = 10.0f * Weight;
		Value2 = 1.0f * Weight;
		AttributeSet->SetMaxHealth(AttributeSet->GetMaxHealth() - Value);
		if (AttributeSet->GetMaxHealth() < AttributeSet->GetHealth())
		{
			AttributeSet->SetHealth(AttributeSet->GetMaxHealth());
		}
		AttributeSet->SetAttackPower(AttributeSet->GetAttackPower() - Value2);
		break;
	case EItemAbility::Stamina:
		Value = 10.0f * Weight;
		AttributeSet->SetMaxStamina(AttributeSet->GetMaxStamina() - Value);
		if (AttributeSet->GetMaxStamina() < AttributeSet->GetStamina())
		{
			AttributeSet->SetStamina(AttributeSet->GetMaxStamina());
		}
		break;
	case EItemAbility::Defence:
		Value = 1.0f * Weight;
		AttributeSet->SetDefense(AttributeSet->GetDefense() - Value);
		break;
	case EItemAbility::Gold:
		Value = 50.0f * Weight;
		Character->AddGoldWeight(-Value);
		break;
	case EItemAbility::Drain:
		Value = 0.5f * Weight;
		if (AProjectERNCharacter* PlayerCharacter = Cast<AProjectERNCharacter>(Character))
		{
			PlayerCharacter->LifestealFraction -= Value;
			if (PlayerCharacter->LifestealFraction < 0.0f)
			{
				PlayerCharacter->LifestealFraction = 0.0f;
			}
		}
		break;
	case EItemAbility::HealthCurse:
		Value = 50.0f * Weight;
		Value2 = 5.0f * Weight;
		AttributeSet->SetMaxHealth(AttributeSet->GetMaxHealth() + Value);
		AttributeSet->SetHealth(AttributeSet->GetHealth() + Value);
		AttributeSet->SetAttackPower(AttributeSet->GetAttackPower() - Value2);
		break;
	case EItemAbility::AttackCurse:
		Value = 5.0f * Weight;
		Value2 = 1.0f * Weight;
		AttributeSet->SetAttackPower(AttributeSet->GetAttackPower() + Value);
		AttributeSet->SetDefense(AttributeSet->GetDefense() - Value2);
		AttributeSet->SetMaxHealth(AttributeSet->GetMaxHealth() - Value2 * 20.0f);
		if (AttributeSet->GetMaxHealth() < AttributeSet->GetHealth())
		{
			AttributeSet->SetHealth(AttributeSet->GetMaxHealth());
		}
	default:
		break;
	}
}
