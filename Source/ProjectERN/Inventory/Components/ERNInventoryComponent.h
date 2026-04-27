// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Inventory/Data/ERNInventoryList.h"
#include "ERNInventoryComponent.generated.h"

class AERNItemActor;
class UItemManagerSubsystem;
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
	// Inventory
	UPROPERTY(Replicated, BlueprintReadOnly, Category="Inventory")
	FInventoryList Inventory;
	
	// Inventory Size
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Inventory")
	int32 MaxSlotSize = 10;
	
};
