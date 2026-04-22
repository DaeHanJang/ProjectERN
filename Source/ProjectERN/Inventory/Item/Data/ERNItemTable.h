// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ERNItemEnums.h"
#include "Engine/DataTable.h"
#include "ERNItemTable.generated.h"

class UItemDataAssetBase;

USTRUCT(BlueprintType)
struct FItemShopPrice
{
	GENERATED_BODY()
	
	// 상점 종류
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EShopType ShopType = EShopType::None;
	
	// 구매 가격
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", UIMin="0"))
	int32 BuyPrice = 0;
};

/**
 * 
 */
USTRUCT(BlueprintType)
struct FERNItemTable : public FTableRowBase
{
	GENERATED_BODY()
	
public:
	// 외부 데이터(cvs/json)가 import될 때
	virtual void OnPostDataImport(const UDataTable* InDataTable, const FName InRowName, TArray<FString>& OutCollectedImportProblems) override;
	// 에디터에서 변경될 때
	virtual void OnDataTableChanged(const UDataTable* InDataTable, const FName InRowName) override;
	
public:
	// 이름
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText DisplayName = FText::GetEmpty();
	
	// 종류
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EItemType ItemType = EItemType::None;
	
	// 등급
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EItemGrade Grade = EItemGrade::None;
	
	// 최대 보유 수량
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1", ClampMax="99", UIMin="1", UIMax="99"))
	int32 MaxStackSize = 1;
	
	// 상점 종류에 따른 구매 가격
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(TitleProperty="ShopType"))
	TArray<FItemShopPrice> ShopPrices;
	
	// 검증 성공 시 불러올 DataAsset(Icon Texture, Mesh 등)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftObjectPtr<UItemDataAssetBase> DataAsset = nullptr;
	
};
