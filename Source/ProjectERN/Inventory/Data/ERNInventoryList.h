#pragma once

#include "CoreMinimal.h"
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
	UPROPERTY()
	FName ItemID = NAME_None;
	
	// 인벤토리 슬롯
	UPROPERTY()
	int32 SlotIndex = INDEX_NONE;
	
	// 보유 수량
	UPROPERTY()
	int32 StackCount = INDEX_NONE;
	
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
	
	// 아이템 추가
	void AddItem();
	// 아이템 제거
	void RemoveItem();
	
public:
	// 인벤토리
	UPROPERTY()
	TArray<FInventoryItemEntry> Items;
	
};

template<>
struct TStructOpsTypeTraits<FInventoryList> : public TStructOpsTypeTraitsBase2<FInventoryList>
{
	enum
	{
		WithNetDeltaSerializer = true
	};
};