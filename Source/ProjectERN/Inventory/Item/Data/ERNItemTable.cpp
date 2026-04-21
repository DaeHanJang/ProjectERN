// Fill out your copyright notice in the Description page of Project Settings.

#include "Inventory/Item/Data/ERNItemTable.h"

void FERNItemTable::OnPostDataImport(const UDataTable* InDataTable, const FName InRowName,
	TArray<FString>& OutCollectedImportProblems)
{
	FTableRowBase::OnPostDataImport(InDataTable, InRowName, OutCollectedImportProblems);
	
	TSet<EShopType> SeenShopTypes;
	
	for (const FItemShopPrice& ShopPrice : ShopPrices)
	{
		// import된 ShopPrices의 값 중 None이 있을 경우
		if (ShopPrice.ShopType == EShopType::None)
		{
			OutCollectedImportProblems.Add(
				FString::Printf(TEXT("Row '%s' has invalid ShopType None."),
					*InRowName.ToString()));
			continue;
		}
		
		// import된 ShopPrices의 값 중 중복이 있을 경우
		if (SeenShopTypes.Contains(ShopPrice.ShopType))
		{
			OutCollectedImportProblems.Add(
				FString::Printf(TEXT("Row '%s' has duplicated ShopType '%s'."), 
					*InRowName.ToString(),
					*UEnum::GetValueAsString(ShopPrice.ShopType)));
			continue;
		}
		
		SeenShopTypes.Add(ShopPrice.ShopType);
	}
}

void FERNItemTable::OnDataTableChanged(const UDataTable* InDataTable, const FName InRowName)
{
	FTableRowBase::OnDataTableChanged(InDataTable, InRowName);
	
	TSet<EShopType> SeenShopTypes;
	TArray<FItemShopPrice> UniqueShopPrices;
	// 미리 적당한 양의 메모리 확보
	UniqueShopPrices.Reserve(ShopPrices.Num() + 1);
	
	for (const FItemShopPrice& ShopPrice : ShopPrices)
	{
		if (ShopPrice.ShopType == EShopType::None)
		{
			continue;
		}
		
		if (SeenShopTypes.Contains(ShopPrice.ShopType))
		{
			continue;
		}
		
		SeenShopTypes.Add(ShopPrice.ShopType);
		UniqueShopPrices.Add(ShopPrice);
	}
 
	// 구조체이므로 복사보다 Move semantics로 처리
	ShopPrices = MoveTemp(UniqueShopPrices);
}
