// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Inventory/Data/ERNInventoryList.h"
#include "ERNInventoryComponent.generated.h"

class AERNItemActor;
class UItemManagerSubsystem;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInventorySlotChanged, const FInventoryItemEntry&, Entry);

/**
 * ERNInventoryComponent - 플레이어 인벤토리 관리
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTERN_API UERNInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UERNInventoryComponent();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	// Getter
	FORCEINLINE int32 GetMaxStackSize() const { return MaxSlotSize; }
	FORCEINLINE const FInventoryList& GetInventory() const { return Inventory; }
	FORCEINLINE FInventoryList& GetInventory() { return Inventory; }
	FORCEINLINE const int32 GetItemQuantity(const int32 SlotIndex) const { return Inventory.GetItemQuantity(SlotIndex); }
	
	// Add Item
	UFUNCTION(Server, Reliable, BlueprintCallable, Category="Inventory")
	void Server_AddItem(AERNItemActor* ItemActor);
	
	// Remove Item
	UFUNCTION(Server, Reliable, BlueprintCallable, Category="Inventory")
	void Server_RemoveItem(const int32 SlotIndex, const int32 Count);
	
protected: 
	virtual void BeginPlay() override;
	
private:
	// Get ItemManager
	UFUNCTION(BlueprintCallable, Category="Inventory")
	UItemManagerSubsystem* GetItemManager() const;

public:
	// Inventory Changed Event
	UPROPERTY(BlueprintAssignable)
	FOnInventorySlotChanged OnInventorySlotChanged;
	
private:
	// Inventory
	UPROPERTY(Replicated, BlueprintReadOnly, Category="Inventory", meta=(AllowPrivateAccess="true"))
	FInventoryList Inventory;
	
	// Inventory Size
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Inventory", meta=(AllowPrivateAccess="true"))
	int32 MaxSlotSize = 10;
	
};
