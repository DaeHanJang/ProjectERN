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
		Items[SlotIndex].SetItemRuntimeState(ItemRuntimeState);
		
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
	OutDropRuntimeState.SetItemAbility(Items[SlotIndex].GetItemRuntimeState().GetItemAbility());
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

void FInventoryList::MoveItem(const int32 FromSlot, const int32 ToSlot, const int32 MaxStackSize)
{
	if (FromSlot == ToSlot) return;
	if (!Items.IsValidIndex(FromSlot) || !Items.IsValidIndex(ToSlot)) return;

	FInventoryItemEntry& From = Items[FromSlot];
	FInventoryItemEntry& To = Items[ToSlot];

	// 같은 아이템 + 스택 가능 → 병합 시도 (대상 슬롯을 가득 채우고 남은 만큼은 출발 슬롯에 잔류)
	if (To.IsValid() && From.GetItemID() == To.GetItemID() && MaxStackSize > 1)
	{
		const int32 Space = MaxStackSize - To.GetQuantity();
		const int32 Transfer = FMath::Min(Space, From.GetQuantity());
		if (Transfer > 0)
		{
			To.AddQuantity(Transfer);
			From.AddQuantity(-Transfer);
			if (From.GetQuantity() <= 0)
			{
				From.Init();
			}
			MarkItemDirty(From);
			MarkItemDirty(To);
			return;
		}
		// Transfer == 0 (대상이 이미 가득 참) → 아래 스왑 로직으로 진행
	}

	// 그 외: 위치 스왑 (빈 칸으로의 이동 포함, 각 엔트리의 SlotIndex는 유지)
	const FItemRuntimeState FromState = From.GetItemRuntimeState();
	const FItemRuntimeState ToState = To.GetItemRuntimeState();

	if (ToState.IsValid())   From.SetItemRuntimeState(ToState);   else From.Init();
	if (FromState.IsValid()) To.SetItemRuntimeState(FromState);   else To.Init();

	MarkItemDirty(From);
	MarkItemDirty(To);
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
}
