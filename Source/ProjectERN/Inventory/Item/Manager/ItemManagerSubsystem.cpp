// Fill out your copyright notice in the Description page of Project Settings.

#include "Inventory/Item/Manager/ItemManagerSubsystem.h"

void UItemManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	checkf(ItemTable, TEXT("ItemTable is not assigned in ItemManagerSubsystem."));
}
