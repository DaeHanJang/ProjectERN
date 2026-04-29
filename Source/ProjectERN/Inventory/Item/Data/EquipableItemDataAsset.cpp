// Fill out your copyright notice in the Description page of Project Settings.

#include "Inventory/Item/Data/EquipableItemDataAsset.h"

void UEquipableItemDataAsset::GatherSoftPaths(const EItemAssetLoadFlags LoadFlags, TArray<FSoftObjectPath>& OutPaths) const
{
	Super::GatherSoftPaths(LoadFlags, OutPaths);
	
	if (EnumHasAnyFlags(LoadFlags, EItemAssetLoadFlags::Gameplay))
	{
		if (!WeaponSkillAbility.IsNull())
		{
			OutPaths.AddUnique(WeaponSkillAbility.ToSoftObjectPath());
		}
	}
}
