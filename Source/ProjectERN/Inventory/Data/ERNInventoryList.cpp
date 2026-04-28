#include "Inventory/Data/ERNInventoryList.h"

void FInventoryItemEntry::PostReplicateAdd(const FInventoryList& InArraySerializer)
{
	for (const FInventoryItemEntry& Entry : InArraySerializer.Items)
	{
		UE_LOG(LogTemp, Warning, TEXT("slot: %d, itemID: %s, quantity: %d"), 
			Entry.SlotIndex, *Entry.ItemID.ToString(), Entry.Quantity);
	}
}

void FInventoryItemEntry::PostReplicateChange(const FInventoryList& InArraySerializer)
{
	for (const FInventoryItemEntry& Entry : InArraySerializer.Items)
	{
		UE_LOG(LogTemp, Warning, TEXT("slot: %d, itemID: %s, quantity: %d"), 
			Entry.SlotIndex, *Entry.ItemID.ToString(), Entry.Quantity);
	}
}

void FInventoryItemEntry::PreReplicatedRemove(const FInventoryList& InArraySerializer)
{
	for (const FInventoryItemEntry& Entry : InArraySerializer.Items)
	{
		UE_LOG(LogTemp, Warning, TEXT("slot: %d, itemID: %s, quantity: %d"), 
			Entry.SlotIndex, *Entry.ItemID.ToString(), Entry.Quantity);
	}
}

bool FInventoryList::AddItem(FItemRuntimeState& ItemRuntimeState, const int32 MaxSlotSize, const int32 MaxStackSize)
{
	// 매개 변수 유효성 검사
	if (ItemRuntimeState.ItemID.IsNone() || ItemRuntimeState.Quantity <= 0 || MaxSlotSize <= 0 || MaxStackSize <= 0)
	{
		return false;
	}
	
	int32 Remaining = ItemRuntimeState.Quantity;
	
	// 기존 슬롯에 추가
	for (FInventoryItemEntry& Entry : Items)
	{
		if (Entry.ItemID != ItemRuntimeState.ItemID)
		{
			continue;
		}
		
		// 현재 슬롯의 채울 수 있는 수량
		const int32 EntrySpace = MaxStackSize - Entry.Quantity;
		// 현재 슬롯에 채울 수량
		const int32 AddQuantity = FMath::Min(EntrySpace, Remaining);
		
		Entry.Quantity += AddQuantity;
		Remaining -= AddQuantity;
		
		MarkItemDirty(Entry);
		
		// 인벤토리에 넣을 아이템을 전부 넣었다면
		if (Remaining <= 0)
		{
			return true;
		}
	}
	
	// 새 슬롯 생성
	while (Remaining > 0)
	{
		// 비어있는 첫 번재 슬롯 인덱스 반환
		const int32 NewSlotIndex = FindFirstEmptySlot(MaxSlotSize);
		if (NewSlotIndex == INDEX_NONE)
		{
			break;
		}
		
		FInventoryItemEntry& NewEntry = Items.AddDefaulted_GetRef();
		NewEntry.ItemID = ItemRuntimeState.ItemID;
		NewEntry.SlotIndex = NewSlotIndex;
		NewEntry.Quantity = FMath::Min(MaxStackSize, Remaining);
		
		Remaining -= NewEntry.Quantity;
		
		OccupiedSlots.Add(NewSlotIndex);
		
		MarkItemDirty(NewEntry);
	}
	
	// 인벤토리에 아이템이 수납되어 Remaining이 변경되었다면
	if (Remaining != ItemRuntimeState.Quantity)
	{
		MarkArrayDirty();
		ItemRuntimeState.Quantity = Remaining;
		return true;
	}
	
	return false;
}

void FInventoryList::RemoveItem()
{
}

int32 FInventoryList::FindFirstEmptySlot(const int32 MaxSlotSize) const
{
	for (int32 Slot = 0; Slot < MaxSlotSize; ++Slot)
	{
		if (!OccupiedSlots.Contains(Slot))
		{
			return Slot;
		}
	}
	
	return INDEX_NONE;
}

void FInventoryList::LogInventory() const
{
	UE_LOG(LogTemp, Warning, TEXT("=================================================="));
	if (Items.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("Inventory is Empty"));
	}
	else
	{
		for (const FInventoryItemEntry& Entry : Items)
		{
			UE_LOG(LogTemp, Warning, TEXT("Slot: %d, ItemID: %s, Quantity: %d"), Entry.SlotIndex, *Entry.ItemID.ToString(), Entry.Quantity);
		}
	}
	UE_LOG(LogTemp, Warning, TEXT("=================================================="));
}
