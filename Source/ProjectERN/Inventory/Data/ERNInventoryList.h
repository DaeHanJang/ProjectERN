#pragma once

#include "CoreMinimal.h"
#include "Inventory/Item/Data/ERNItemRuntimeState.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "ERNInventoryList.generated.h"

class UERNInventoryComponent;
struct FInventoryList;

// Inventory Item Entry
USTRUCT(BlueprintType)
struct FInventoryItemEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()
	
public:
	// 클라에서 새 아이템 추가됨
	void PostReplicatedAdd(const FInventoryList& InArraySerializer);
	// 클라에서 아이템 수정됨
	void PostReplicatedChange(const FInventoryList& InArraySerializer);
	// 클라에서 아이템 제거 직전
	void PreReplicatedRemove(const FInventoryList& InArraySerializer);
	
public:
	// 아이템 고유 Key값
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FName ItemID = NAME_None;
	
	// 인벤토리 슬롯
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	int32 SlotIndex = INDEX_NONE;
	
	// 보유 수량
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	int32 Quantity = 0;
	
	// 아이템 런타임 상태값 구조체
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FItemRuntimeState ItemRuntimeState;
	
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
	UERNInventoryComponent* GetOwner() const { return Owner.Get(); }
	void SetOwner(UERNInventoryComponent* NewOwner);
	const TArray<FInventoryItemEntry>& GetItems() const { return Items; }
	
	// Add Item
	bool AddItem(FItemRuntimeState& ItemRuntimeState, const int32 MaxSlotSize, const int32 MaxStackSize, TArray<FInventoryItemEntry>& OutChangedEntries);
	
	// TODO: RemoveItem 함수 구현
	// Remove Item
	void RemoveItem();
	
	// Debug Log
	void LogInventory() const;
	
private:
	// 비어있는 첫 번재 슬롯 인덱스 반환
	int32 FindFirstEmptySlot(const int32 MaxSlotSize) const;
	
private:
	// Inventory Container
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess=true))
	TArray<FInventoryItemEntry> Items;
	
	// 현재 사용중인 슬롯 집합
	TSet<int32> OccupiedSlots;
	
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
