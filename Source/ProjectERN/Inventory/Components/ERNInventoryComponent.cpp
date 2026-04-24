// Copyright Epic Games, Inc. All Rights Reserved.

#include "Inventory/Components/ERNInventoryComponent.h"
#include "Net/UnrealNetwork.h"

UERNInventoryComponent::UERNInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UERNInventoryComponent::BeginPlay()
{
	Super::BeginPlay();
}

bool UERNInventoryComponent::IsServerSide() const
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return false;
	}
	
	return true;
}

void UERNInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UERNInventoryComponent, Inventory);
}

void UERNInventoryComponent::Server_AddItem_Implementation(FName ItemID, FInventoryItemEntry Item)
{
	if (!IsServerSide())
	{
		return;
	}
	
	// TODO: 아이템 추가 로직
}

void UERNInventoryComponent::Server_RemoveItem_Implementation(int32 SlotIndex, int32 Count)
{
	if (!IsServerSide())
	{
		return;
	}
	
	// TODO: 아이템 제거 로직
	UE_LOG(LogTemp, Log, TEXT("Server_RemoveItem: Slot %d, Count %d"), SlotIndex, Count);
}

void UERNInventoryComponent::Server_UseItem_Implementation(int32 SlotIndex)
{
	if (!IsServerSide())
	{
		return;
	}
	
	// TODO: 아이템 사용 로직
	UE_LOG(LogTemp, Log, TEXT("Server_UseItem: Slot %d"), SlotIndex);
}
