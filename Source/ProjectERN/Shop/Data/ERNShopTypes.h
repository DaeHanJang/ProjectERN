
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Inventory/Item/Data/ERNItemEnums.h"
#include "Inventory/Item/Data/ERNItemRuntimeState.h"
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

    // 4. 무작위 등장 확률 가중치 (높을수록 잘 등장)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop", meta=(ClampMin="0.0"))
    float SpawnWeight = 50.0f;

    // 5. 몇 개나 파는가? (-1: 무제한, 0 이상: 제한 수량)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop")
    int32 MaxStock = -1;

    // 6. 최소 등장 보장 여부 (true면 랜덤을 무시하고 최우선 배치)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop")
    bool bGuaranteed = false;

    // 7. 무료 제공 여부 (true면 기존 아이템 테이블의 가격을 무시하고 0원에 판매)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop")
    bool bIsFree = false;

    // 8. 가격 배율 (예: 1.5 = 1.5배 가격, 0.5 = 반값 할인)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop", meta=(ClampMin="0.0"))
    float PriceMultiplier = 1.0f;
};

/**
 * 상인의 카테고리별 진열 슬롯 수량 설정
 */
USTRUCT(BlueprintType)
struct FERNShopSlotConfig
{
    GENERATED_BODY()

    /** 대상 카테고리 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop Config")
    EItemType Category = EItemType::Equipable;

    /** 해당 카테고리에서 선정할 랜덤 아이템 수 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop Config", meta = (ClampMin = "0", ClampMax = "20"))
    int32 SlotCount = 3;
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
    
    // 무작위 어빌리티 등 런타임 정보 저장을 위한 상태 변수
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop")
    FItemRuntimeState ItemState;

    // UI에서 카테고리별 탭(Tab) 분류 및 정렬을 위해 추가
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop")
    EItemType ItemCategory = EItemType::Equipable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop")
    FGuid UniqueID;

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

    /** 구매 대상 상인의 고유 식별자 */
    UPROPERTY(BlueprintReadWrite, Category = "Shop")
    FName ShopID;

    UPROPERTY(BlueprintReadWrite, Category = "Shop")
    EShopType ShopType = EShopType::None;

    UPROPERTY(BlueprintReadWrite, Category = "Shop")
    FName ItemID;

    UPROPERTY(BlueprintReadWrite, Category = "Shop")
    FGuid UniqueID;

    UPROPERTY(BlueprintReadWrite, Category = "Shop")
    int32 Quantity = 1;

    UPROPERTY(BlueprintReadWrite, Category = "Shop")
    int32 TotalPrice = 0;  // 룬 단위

    UPROPERTY(BlueprintReadWrite, Category = "Shop")
    EERNTransactionResult Result = EERNTransactionResult::Success;

    UPROPERTY(BlueprintReadWrite, Category = "Shop")
    float Timestamp = 0.f;

    // 구매한 플레이어 캐릭터 (멀티플레이어 트랜잭션 식별용)
    UPROPERTY(BlueprintReadWrite, Category = "Shop")
    AActor* Buyer = nullptr;
};

/**
 * 상점 인벤토리 - 특정 상점의 전체 아이템 목록
 */
USTRUCT(BlueprintType)
struct FERNShopInventory
{
    GENERATED_BODY()

    /** 상인별 고유 식별자 (캐시 관리용 Key) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop")
    FName ShopID;

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
