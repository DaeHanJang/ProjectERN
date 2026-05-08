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
