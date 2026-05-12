#include "Inventory/Data/ERNInventoryList.h"

#include "Inventory/Components/ERNInventoryComponent.h"

void FInventoryItemEntry::PostReplicatedAdd(const FInventoryList& InArraySerializer)
{
	if (const UERNInventoryComponent* InventoryComponent = InArraySerializer.GetOwner())
	{
		InventoryComponent->OnInventorySlotChanged.Broadcast(*this);
	}
}

void FInventoryItemEntry::PostReplicatedChange(const FInventoryList& InArraySerializer)
{
	if (const UERNInventoryComponent* InventoryComponent = InArraySerializer.GetOwner())
	{
		InventoryComponent->OnInventorySlotChanged.Broadcast(*this);
	}
}

void FInventoryItemEntry::PreReplicatedRemove(const FInventoryList& InArraySerializer)
{
	if (const UERNInventoryComponent* InventoryComponent = InArraySerializer.GetOwner())
	{
		InventoryComponent->OnInventorySlotChanged.Broadcast(*this);
	}
}

void FInventoryItemEntry::Init()
{
	ItemID = NAME_None;
	ItemRuntimeState.Init();
}

void FInventoryList::SetOwner(UERNInventoryComponent* NewOwner)
{
	if (NewOwner)
	{
		Owner = NewOwner;
	}
}

const int32 FInventoryList::GetItemQuantity(const int32 SlotIndex) const
{
	if (!Items.IsValidIndex(SlotIndex))
	{
		return INDEX_NONE;
	}
	
	return Items[SlotIndex].GetQuantity();
}

void FInventoryList::Init(const int32 Size)
{
	Items.Reserve(Size);
	Items.SetNum(Size);
	for (int32 i = 0; i < Size; ++i)
	{
		Items[i].Init();
		Items[i].SetSlotIndex(i);
		MarkItemDirty(Items[i]);
	}
	MarkArrayDirty();
}

bool FInventoryList::AddItem(FItemRuntimeState& ItemRuntimeState, const int32 MaxSlotSize, const int32 MaxStackSize, TArray<FInventoryItemEntry>& OutChangedEntries)
{
	// 매개 변수 유효성 검사
	if (!ItemRuntimeState.IsValid() || ItemRuntimeState.GetQuantity() <= 0 || MaxSlotSize <= 0 || MaxStackSize <= 0)
	{
		return false;
	}
	
	OutChangedEntries.Empty();
	int32 Remaining = ItemRuntimeState.GetQuantity();
	
	// 기존 슬롯에 추가
	for (FInventoryItemEntry& Entry : Items)
	{
		if (!Entry.IsValid() || Entry.GetItemID() != ItemRuntimeState.GetItemID())
		{
			continue;
		}
		
		// 현재 슬롯의 채울 수 있는 수량
		const int32 EntrySpace = MaxStackSize - Entry.GetQuantity();
		// 현재 슬롯에 채울 수량
		const int32 AddQuantity = FMath::Min(EntrySpace, Remaining);
		if (EntrySpace <= 0)
		{
			continue;
		}
		
		Entry.AddQuantity(AddQuantity);
		Remaining -= AddQuantity;
		
		OutChangedEntries.Add(Entry);
		MarkItemDirty(Entry);
		
		// 인벤토리에 넣을 아이템을 전부 넣었다면
		if (Remaining <= 0)
		{
			ItemRuntimeState.SetQuantity(Remaining);
			return true;
		}
	}
	
	// 새 슬롯 생성
	while (Remaining > 0)
	{
		// 비어있는 첫 번재 슬롯 인덱스 반환
		const int32 SlotIndex = FindFirstEmptySlot(MaxSlotSize);
		if (SlotIndex == INDEX_NONE)
		{
			break;
		}
		
		Items[SlotIndex].SetItemID(ItemRuntimeState.GetItemID());
		Items[SlotIndex].SetQuantity(FMath::Min(MaxStackSize, Remaining));
		
		Remaining -= Items[SlotIndex].GetQuantity();
		
		OutChangedEntries.Add(Items[SlotIndex]);
		MarkItemDirty(Items[SlotIndex]);
	}
	
	// 인벤토리에 아이템이 수납되어 Remaining이 변경되었다면
	if (Remaining != ItemRuntimeState.GetQuantity())
	{
		ItemRuntimeState.SetQuantity(Remaining);
		
		return true;
	}
	
	return false;
}

const int32 FInventoryList::FindFirstEmptySlot(const int32 MaxSlotSize) const
{
	for (int32 i = 0; i < MaxSlotSize; ++i)
	{
		if (!Items[i].IsValid())
		{
			return i;
		}
	}
	
	return INDEX_NONE;
}

void FInventoryList::RemoveItem(const int32 SlotIndex, const int32 Count, FItemRuntimeState& OutDropRuntimeState, FInventoryItemEntry& OutChangedEntry)
{
	// 인벤토리에 아이템을 지우고 월드에 생성할 ItemActor의 RuntimeState 값 설정
	OutDropRuntimeState.SetItemID(Items[SlotIndex].GetItemID());
	OutDropRuntimeState.SetQuantity(Count);
	Items[SlotIndex].AddQuantity(-Count);
	// 슬롯에 남아있는 아이템의 수량이 0이하라면 슬롯 초기화
	if (Items[SlotIndex].GetQuantity() <= 0)
	{
		Items[SlotIndex].Init();
	}
	MarkItemDirty(Items[SlotIndex]);
	// 리슨 서버 UI 갱신용 슬롯 데이터 적재
	OutChangedEntry = Items[SlotIndex];
}

FItemRuntimeState FInventoryList::ChangeItem(const int32 SlotIndex, const FItemRuntimeState& NewItemRuntimeState)
{
	FItemRuntimeState OutItemRuntimeState = Items[SlotIndex].GetItemRuntimeState();
	
	Items[SlotIndex].SetItemRuntimeState(NewItemRuntimeState);
	MarkItemDirty(Items[SlotIndex]);
	
	return OutItemRuntimeState;
}

void FInventoryList::CopyFrom(const FInventoryList& SourceInventory, const int32 Size)
{
	Items.Empty();
	Items.SetNum(Size);
	
	const TArray<FInventoryItemEntry>& SourceItems = SourceInventory.GetItems();
	
	for (int32 i = 0; i < Size; ++i)
	{
		if (SourceItems.IsValidIndex(i))
		{
			Items[i] = SourceItems[i];
		}
		else
		{
			Items[i].Init();
		}
		
		Items[i].SetSlotIndex(i);
		MarkItemDirty(Items[i]);
	}
	
	MarkArrayDirty();
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
			UE_LOG(LogTemp, Warning, TEXT("Slot: %d, ItemID: %s, Quantity: %d"), Entry.GetSlotIndex(), *Entry.GetItemID().ToString(), Entry.GetQuantity());
		}
	}
	UE_LOG(LogTemp, Warning, TEXT("=================================================="));
	
	GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green, TEXT("=================================================="));
	if (Items.IsEmpty())
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green, TEXT("Inventory is Empty"));
	}
	else
	{
		for (const FInventoryItemEntry& Entry : Items)
		{
			GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green, FString::Printf(TEXT("Slot: %d, ItemID: %s, Quantity: %d"), Entry.GetSlotIndex(), *Entry.GetItemID().ToString(), Entry.GetQuantity()));
		}
	}
	GEngine->AddOnScreenDebugMessage(-1, 3.0f, FColor::Green, TEXT("=================================================="));
}
