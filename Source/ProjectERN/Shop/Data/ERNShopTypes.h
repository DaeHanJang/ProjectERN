// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "ERNShopTypes.generated.h"

// ===== 로그 카테고리 =====
DECLARE_LOG_CATEGORY_EXTERN(LogShopModel, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogShopProvider, Log, All);

// ===== 열거형 =====

UENUM(BlueprintType)
enum class EERNShopItemCategory : uint8
{
    Weapon      UMETA(DisplayName = "무기"),
    Armor       UMETA(DisplayName = "방어구"),
    Consumable  UMETA(DisplayName = "소모품"),
};

UENUM(BlueprintType) 
enum class EERNTransactionResult : uint8
{
    Success,
    InsufficientFunds,
    OutOfStock,
    InventoryFull,
    NotAuthorized,
    InvalidItem,
    NetworkError,
};

// ===== 구조체 =====

/**
 * 상점 아이템 데이터 - 개별 아이템의 모든 정보를 담는 구조체
 */
USTRUCT(BlueprintType)
struct FERNShopItemData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop")
    FName ItemID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop")
    FText Description;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop")
    int32 Price = 0;  // 룬 단위

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop")
    EERNShopItemCategory Category = EERNShopItemCategory::Weapon;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop")
    int32 MaxStack = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop")
    int32 StockCount = -1;  // -1 = 무제한

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop")
    bool bIsAvailable = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop")
    TSoftObjectPtr<UTexture2D> Icon;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop")
    FGameplayTagContainer RequiredTags;
};

/**
 * 상점 카테고리 데이터 - UI 탭 정보
 */
USTRUCT(BlueprintType)
struct FERNShopCategoryData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop")
    EERNShopItemCategory Category = EERNShopItemCategory::Weapon;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop")
    int32 SortOrder = 0;
};

/**
 * 상점 거래 데이터 - 구매 요청 및 결과 (판매 기능 없음)
 */
USTRUCT(BlueprintType)
struct FERNShopTransaction
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, Category = "Shop")
    FName ShopID;

    UPROPERTY(BlueprintReadWrite, Category = "Shop")
    FName ItemID;

    UPROPERTY(BlueprintReadWrite, Category = "Shop")
    int32 Quantity = 1;

    UPROPERTY(BlueprintReadWrite, Category = "Shop")
    int32 TotalPrice = 0;  // 룬 단위

    UPROPERTY(BlueprintReadWrite, Category = "Shop")
    EERNTransactionResult Result = EERNTransactionResult::Success;

    UPROPERTY(BlueprintReadWrite, Category = "Shop")
    float Timestamp = 0.f;
};

/**
 * 상점 인벤토리 - 특정 상점의 전체 아이템 목록
 */
USTRUCT(BlueprintType)
struct FERNShopInventory
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop")
    FName ShopID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop")
    TArray<FERNShopItemData> Items;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop")
    int32 ShopLevel = 1;

    UPROPERTY(BlueprintReadWrite, Category = "Shop")
    float RefreshTimestamp = 0.f;

    /** 공유 인벤토리 여부 - true이면 파티 전체가 동일 재고 풀을 공유 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop")
    bool bIsSharedPool = true;
};


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnShopDataReceived, const FERNShopInventory&, ShopData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPurchaseComplete, const FERNShopTransaction&, Transaction);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnShopError, const FString&, ErrorMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnShopUIUpdate);
