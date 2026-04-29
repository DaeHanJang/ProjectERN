// Fill out your copyright notice in the Description page of Project Settings.

#include "Inventory/Item/Data/ConsumableItemDataAsset.h"

void UConsumableItemDataAsset::GatherSoftPaths(const EItemAssetLoadFlags LoadFlags, TArray<FSoftObjectPath>& OutPaths) const
{
	Super::GatherSoftPaths(LoadFlags, OutPaths);
	
	if (EnumHasAnyFlags(LoadFlags, EItemAssetLoadFlags::Gameplay))
	{
		if (ConsumableType == EConsumableType::Usable && !ConsumableAbility.IsNull())
		{
			OutPaths.AddUnique(ConsumableAbility.ToSoftObjectPath());
		}
	}
}
