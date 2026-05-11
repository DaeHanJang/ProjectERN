// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/ERNUIFunctionLibrary.h"

FText UERNUIFunctionLibrary::GetItemTypeText(EItemType ItemType)
{
	switch (ItemType)
	{
	case EItemType::Equipable:
		return FText::FromString(TEXT("장비"));
	case EItemType::Consumable:
		return FText::FromString(TEXT("소모품"));
	default:
		return FText::GetEmpty();
	}
}

FText UERNUIFunctionLibrary::GetWeaponTypeText(EWeaponType WeaponType)
{
	switch (WeaponType)
	{
	case EWeaponType::Sword:
		return FText::FromString(TEXT("검"));
	case EWeaponType::Staff:
		return FText::FromString(TEXT("지팡이"));
	case EWeaponType::Polearm:
		return FText::FromString(TEXT("폴암"));
	default:
		return FText::GetEmpty();
	}
}

FText UERNUIFunctionLibrary::GetConsumableTypeText(EConsumableType ConsumableType)
{
	switch (ConsumableType)
	{
	case EConsumableType::Usable:
		return FText::FromString(TEXT("사용 가능"));
	case EConsumableType::Unusable:
		return FText::FromString(TEXT("재료"));
	default:
		return FText::GetEmpty();
	}
}
