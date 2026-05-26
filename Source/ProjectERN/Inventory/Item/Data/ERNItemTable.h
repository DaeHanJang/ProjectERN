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
	
public:
	// 이름
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText DisplayName = FText::GetEmpty();
	
	// 상세 설명
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText Description = FText::GetEmpty();
	
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
	
	// ===== 강화(Upgrade) 관련 필드 =====
	
	// 강화 시 변환될 다음 등급 아이템의 DT_ItemTable RowName
	// None이면 해당 아이템은 강화 불가 (최종 등급 또는 월드 획득 전용)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade")
	FName NextGradeItemID;
	
	// 강화에 필요한 재료 아이템의 DT_ItemTable RowName (소모 수량은 1개 고정)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade")
	FName UpgradeMaterialID;
	
};
