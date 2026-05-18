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

void UERNUIFunctionLibrary::GetPurchaseResultToastData(EERNTransactionResult Result, FText& OutMessage, FLinearColor& OutColor)
{
	switch (Result)
	{
		case EERNTransactionResult::Success:
			OutMessage = FText::FromString(TEXT("구매 완료!"));
			OutColor = FLinearColor(0.2f, 1.0f, 0.2f); // 초록색
			break;

		case EERNTransactionResult::InsufficientFunds:
			OutMessage = FText::FromString(TEXT("골드가 부족합니다."));
			OutColor = FLinearColor(1.0f, 0.2f, 0.2f); // 빨간색
			break;

		case EERNTransactionResult::InventoryFull:
			OutMessage = FText::FromString(TEXT("인벤토리가 가득 찼습니다."));
			OutColor = FLinearColor(1.0f, 0.8f, 0.2f); // 노란색
			break;

		case EERNTransactionResult::OutOfStock:
			OutMessage = FText::FromString(TEXT("품절된 상품입니다."));
			OutColor = FLinearColor(0.5f, 0.5f, 0.5f); // 회색
			break;

		case EERNTransactionResult::InvalidItem:
		default:
			OutMessage = FText::FromString(TEXT("유효하지 않은 아이템입니다."));
			OutColor = FLinearColor(1.0f, 0.0f, 0.0f); // 짙은 빨간색
			break;
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
