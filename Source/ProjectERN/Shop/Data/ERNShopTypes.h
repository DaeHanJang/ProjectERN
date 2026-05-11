
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Inventory/Item/Data/ERNItemEnums.h"
#include "Engine/DataTable.h"
#include "ERNShopTypes.generated.h"

// ===== 로그 카테고리 =====
DECLARE_LOG_CATEGORY_EXTERN(LogShopModel, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogShopProvider, Log, All);

// ===== 열거형 =====
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
 * 상점 진열용 상품 데이터 구조체 (데이터 테이블용)
 */
USTRUCT(BlueprintType)
struct FERNShopProductTable : public FTableRowBase
{
    GENERATED_BODY()

    // 어느 상점에서 파는가?
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop")
    EShopType ShopType = EShopType::None;

    // 무엇을 파는가? (DT_ItemTable의 RowName과 매핑되는 외래 키)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop")
    FName ItemID;

    // 얼마에 파는가?
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop")
    int32 Price = 0;

    // 몇 개나 파는가? (-1: 무제한, 0 이상: 제한 수량)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop")
    int32 MaxStock = -1;
};

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
    int32 Price = 0;  // 룬 단위

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop")
    int32 StockCount = -1;  // -1 = 무제한

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop")
    bool bIsAvailable = true;
};

/**
 * 상점 카테고리 데이터 - UI 탭 정보
 */
USTRUCT(BlueprintType)
struct FERNShopCategoryData
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop")
    EItemType Category = EItemType::Equipable;

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
    EShopType ShopType = EShopType::None;

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
    EShopType ShopType = EShopType::None;

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
