// Copyright Epic Games, Inc. All Rights Reserved.

#include "Inventory/Components/ERNInventoryComponent.h"

#include "Inventory/Item/ERNItemActor.h"
#include "Inventory/Item/Manager/ItemManagerSubsystem.h"
#include "Net/UnrealNetwork.h"

UERNInventoryComponent::UERNInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	Inventory.SetOwner(this);
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
	
	UItemManagerSubsystem* ItemManager = GetItemManager();
	// 서버에서 ItemID 유효성 검사
	if (!ItemManager || !ItemManager->ItemValid(ItemRuntimeState.ItemID))
	{
		return;
	}
	
	TArray<FInventoryItemEntry> ChangedSlots;
	// Inventory에 넣기 요청
	const bool bAdded = Inventory.AddItem(ItemRuntimeState, MaxSlotSize, ItemManager->FindItemRow(ItemRuntimeState.ItemID)->MaxStackSize, ChangedSlots);
	if (!bAdded)
	{
		return;
	}
	
	// 리슨 서버 인벤토리 변경 이벤트 발신 (UI 갱신)
	for (const FInventoryItemEntry& ChangedSlot : ChangedSlots)
	{
		OnInventorySlotChanged.Broadcast(ChangedSlot);
	}
	
	Inventory.LogInventory();
	
	if (ItemRuntimeState.Quantity <= 0)
	{
		// TODO: ItemActor 제거 함수 넣기
		ItemActor->Destroy();
	}
}

void UERNInventoryComponent::Server_RemoveItem_Implementation(const int32 SlotIndex, const int32 Count)
{
	// 매개 변수 유효성 검사
	if (SlotIndex < 0 || SlotIndex >= MaxSlotSize)
	{
		return;
	}
	if (!Inventory.GetItem(SlotIndex) || Count < 0 || Count > Inventory.GetItem(SlotIndex)->Quantity)
	{
		return;
	}
	
	// 리슨 서버 UI 갱신용 갱신된 인벤토리 슬롯 배열
	FInventoryItemEntry ChangedSlot;
	FItemRuntimeState DropItemRuntimeState;
	// 인벤토리에서 아이템 제거
	Inventory.RemoveItem(SlotIndex, Count, DropItemRuntimeState, ChangedSlot);
	
	// 리슨 서버 전용 UI 갱신 이벤트
	OnInventorySlotChanged.Broadcast(ChangedSlot);
	
	Inventory.LogInventory();
	
	// 인벤토리에서 제거한 아이템 월드에 생성 
	const FVector& DropLocation = GetOwner()->GetActorLocation() + GetOwner()->GetActorForwardVector() * 50.0f;
	GetItemManager()->SpawnItem(DropItemRuntimeState.ItemID, DropItemRuntimeState.Quantity, DropLocation, GetOwner()->GetActorRotation());
}
