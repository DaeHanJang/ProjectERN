// Fill out your copyright notice in the Description page of Project Settings.

#include "Inventory/Item/Data/ItemDataAssetBase.h"

void UItemDataAssetBase::GatherSoftPaths(const EItemAssetLoadFlags LoadFlags, TArray<FSoftObjectPath>& OutPaths) const
{
	if (EnumHasAnyFlags(LoadFlags, EItemAssetLoadFlags::UI) && !Icon.IsNull())
	{
		OutPaths.AddUnique(Icon.ToSoftObjectPath());
	}
}
