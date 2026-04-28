#pragma once

#include "CoreMinimal.h"
#include "Inventory/Item/Data/ERNItemRuntimeState.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "ERNInventoryList.generated.h"

struct FInventoryList;

// Inventory Item Entry
USTRUCT(BlueprintType)
struct FInventoryItemEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()
	
public:
	// 클라에서 새 아이템 추가됨
	void PostReplicateAdd(const FInventoryList& InArraySerializer);
	// 클라에서 아이템 수정됨
	void PostReplicateChange(const FInventoryList& InArraySerializer);
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
	
	// Add Item
	bool AddItem(FItemRuntimeState& ItemRuntimeState, const int32 MaxSlotSize, const int32 MaxStackSize);
	// Remove Item
	void RemoveItem();
	
	// Debug Log
	void LogInventory() const;
	
private:
	// 비어있는 첫 번재 슬롯 인덱스 반환
	int32 FindFirstEmptySlot(const int32 MaxSlotSize) const;
	
public:
	// Inventory Container
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TArray<FInventoryItemEntry> Items;
	
	// 현재 사용중인 슬롯 집합
	TSet<int32> OccupiedSlots;
};

template<>
struct TStructOpsTypeTraits<FInventoryList> : public TStructOpsTypeTraitsBase2<FInventoryList>
{
	enum
	{
		WithNetDeltaSerializer = true
	};
};
