// Fill out your copyright notice in the Description page of Project Settings.

#include "Inventory/Item/Data/ItemDataAssetBase.h"

void UItemDataAssetBase::GatherSoftPaths(const EItemAssetLoadFlags LoadFlags, TArray<FSoftObjectPath>& OutPaths) const
{
	if (EnumHasAnyFlags(LoadFlags, EItemAssetLoadFlags::UI))
	{
		if (!Icon.IsNull())
		{
			OutPaths.AddUnique(Icon.ToSoftObjectPath());
		}
	}
	if (EnumHasAnyFlags(LoadFlags, EItemAssetLoadFlags::Gameplay))
	{
		if (!StaticMesh.IsNull())
		{
			OutPaths.AddUnique(StaticMesh.ToSoftObjectPath());
		}
		else if (!SkeletalMesh.IsNull())
		{
			OutPaths.AddUnique(SkeletalMesh.ToSoftObjectPath());
		}
		if (!Sound.IsNull())
		{
			OutPaths.AddUnique(Sound.ToSoftObjectPath());
		}
	}
}
