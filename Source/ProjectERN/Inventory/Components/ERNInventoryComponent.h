// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Inventory/Data/ERNInventoryList.h"
#include "ERNInventoryComponent.generated.h"

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

protected:
	virtual void BeginPlay() override;

public:
	// 인벤토리
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Inventory")
	FInventoryList Inventory;

	// 아이템 추가
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Inventory")
	void Server_AddItem(FName ItemID, FInventoryItemEntry Item);

	// 아이템 제거
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Inventory")
	void Server_RemoveItem(int32 SlotIndex, int32 Count);

	// 아이템 사용
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Inventory")
	void Server_UseItem(int32 SlotIndex);
	
private:
	// Server Check
	bool IsServerSide() const;
	
};
