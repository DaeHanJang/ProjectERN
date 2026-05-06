// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ERNItemRuntimeState.generated.h"

// Item Runtime State
USTRUCT(BlueprintType)
struct FItemRuntimeState
{
	GENERATED_BODY()
	
public:
	// Getter/Setter
	FORCEINLINE const FName& GetItemID() const { return ItemID; }
	FORCEINLINE void SetItemID(const FName& InItemID) { ItemID = InItemID; }
	FORCEINLINE const int32 GetQuantity() const { return Quantity; }
	FORCEINLINE void SetQuantity(const int32 InQuantity) { Quantity = InQuantity; }
	FORCEINLINE void AddQuantity(const int32 AddQuantity) { Quantity += AddQuantity; }
	
	FORCEINLINE bool IsValid() const { return !ItemID.IsNone(); }
	
	// 초기화
	void Init();
	
private:
	// 아이템 키값
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta=(AllowPrivateAccess="true"))
	FName ItemID = NAME_None;
	
	// 수량
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta=(AllowPrivateAccess="true"))
	int32 Quantity = 0;
	
};
