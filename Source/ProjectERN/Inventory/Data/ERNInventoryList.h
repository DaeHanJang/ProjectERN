#pragma once

#include "CoreMinimal.h"
#include "Inventory/Item/Data/ERNItemRuntimeState.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "ERNInventoryList.generated.h"

struct FInventoryList;
class UERNInventoryComponent;

// Inventory Item Entry
USTRUCT(BlueprintType)
struct FInventoryItemEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()
	
public:
	// Getter/Setter
	FORCEINLINE const FName& GetItemID() const { return ItemID; }
	FORCEINLINE void SetItemID(const FName& NewItemID) { ItemID = NewItemID; ItemRuntimeState.SetItemID(NewItemID); }
	FORCEINLINE const int32 GetSlotIndex() const { return SlotIndex; }
	FORCEINLINE void SetSlotIndex(const int32 NewSlotIndex) { SlotIndex = NewSlotIndex; }
	FORCEINLINE const int32 GetQuantity() const { return ItemRuntimeState.GetQuantity();}
	FORCEINLINE void SetQuantity(const int32 NewQuantity) { ItemRuntimeState.SetQuantity(NewQuantity); }
	FORCEINLINE void AddQuantity(const int32 AddQuantity) { ItemRuntimeState.AddQuantity(AddQuantity); }
	FORCEINLINE const FItemRuntimeState& GetItemRuntimeState() const { return ItemRuntimeState; }
	FORCEINLINE void SetItemRuntimeState(const FItemRuntimeState& NewItemRuntimeState) { ItemRuntimeState = NewItemRuntimeState; ItemID = NewItemRuntimeState.GetItemID(); }
		
	FORCEINLINE bool IsValid() const { return !ItemID.IsNone(); }
	
	// 클라에서 새 아이템 추가됨
	void PostReplicatedAdd(const FInventoryList& InArraySerializer);
	// 클라에서 아이템 수정됨
	void PostReplicatedChange(const FInventoryList& InArraySerializer);
	// 클라에서 아이템 제거 직전
	void PreReplicatedRemove(const FInventoryList& InArraySerializer);
	
	// 초기화
	void Init();
	
private:
	// 아이템 고유 Key값
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(AllowPrivateAccess="true"))
	FName ItemID = NAME_None;
	
	// 인벤토리 슬롯
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(AllowPrivateAccess="true"))
	int32 SlotIndex = INDEX_NONE;
	
	// 아이템 런타임 상태값 구조체
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(AllowPrivateAccess="true"))
	FItemRuntimeState ItemRuntimeState = FItemRuntimeState();
	
};

// Inventory List
USTRUCT(BlueprintType)
struct FInventoryList : public FFastArraySerializer
{
	GENERATED_BODY()
	
public:
	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParams)
	{
		return FastArrayDeltaSerialize<FInventoryItemEntry, FInventoryList>(Items, DeltaParams, *this);
	}
	
	// Getter/Setter
	FORCEINLINE UERNInventoryComponent* GetOwner() const { return Owner.Get(); }
	// ERNInventoryComponent의 FInventoryList Inventory 변수 때문에 cpp에서 구현 
	void SetOwner(UERNInventoryComponent* NewOwner);
	FORCEINLINE const TArray<FInventoryItemEntry>& GetItems() const { return Items;}
	
	// Get Item Quantity
	const int32 GetItemQuantity(const int32 SlotIndex) const;
	
	// Initialization
	void Init(const int Size);
	
	// Add Item
	bool AddItem(FItemRuntimeState& ItemRuntimeState, const int32 MaxSlotSize, const int32 MaxStackSize, TArray<FInventoryItemEntry>& OutChangedEntries);
	
	// Remove Item
	void RemoveItem(const int32 SlotIndex, const int32 Count, FItemRuntimeState& OutDropRuntimeState, FInventoryItemEntry& OutChangedEntry);
	
	// Change Item
	FItemRuntimeState ChangeItem(const int32 SlotIndex, const FItemRuntimeState& NewItemRuntimeState);
	
	// Copy Inventory
	void CopyFrom(const FInventoryList& SourceInventory, const int32 Size);

	// 스냅샷 배열로 복원 (travel 진행 보존용)
	void RestoreFrom(const TArray<FInventoryItemEntry>& InEntries, const int32 Size)
	{
		Items = InEntries;
		if (Items.Num() < Size)
		{
			Items.AddDefaulted(Size - Items.Num());
		}
		for (FInventoryItemEntry& Item : Items)
		{
			MarkItemDirty(Item);
		}
		MarkArrayDirty();
	}
	
	// Debug Log
	void LogInventory() const;
	
private:
	// 비어있는 첫 번재 슬롯 인덱스 반환
	const int32 FindFirstEmptySlot(const int32 MaxSlotSize) const;
	
private:
	// Inventory Container
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess=true))
	TArray<FInventoryItemEntry> Items;
	
	// FInventoryList를 쇼유하고 있는 InventoryComponent
	UPROPERTY(Transient, NotReplicated)
	TWeakObjectPtr<UERNInventoryComponent> Owner = nullptr;
};

template<>
struct TStructOpsTypeTraits<FInventoryList> : public TStructOpsTypeTraitsBase2<FInventoryList>
{
	enum
	{
		WithNetDeltaSerializer = true
	};
};
