#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Shop/Provider/ERNShopDataProvider.h"
#include "ERNDataTableShopProvider.generated.h"

class UDataTable;

/**
 * 인벤토리 팀의 아이템 데이터 테이블을 파싱하여 
 * 상점 인벤토리를 구성하는 실제 Data Provider입니다.
 */
UCLASS(Blueprintable, BlueprintType)
class PROJECTERN_API UERNDataTableShopProvider : public UObject, public IERNShopDataProvider
{
    GENERATED_BODY()

public:
    UERNDataTableShopProvider();

    // ===== IERNShopDataProvider 인터페이스 구현 =====
    virtual void Initialize_Implementation(UObject* Owner) override;
    virtual void RequestShopData_Implementation(EShopType ShopType) override;
    virtual void RequestPurchase_Implementation(FERNShopTransaction Transaction) override;
    virtual FERNShopInventory GetCachedShopData_Implementation(EShopType ShopType) override;
    virtual bool IsDataReady_Implementation() override;
    virtual void HandleReceivedData_Implementation(const FERNShopInventory& ShopData) override;
    virtual void HandlePurchaseResult_Implementation(const FERNShopTransaction& Transaction) override;

    /** 마스터 테이블과 슬롯 설정으로부터 무작위 상점 인벤토리를 생성합니다. */
    UFUNCTION(BlueprintCallable, Category = "Shop|Generation")
    FERNShopInventory GenerateRandomInventory(FName ShopID, EShopType ShopType, const TArray<FERNShopSlotConfig>& SlotConfigs);

    /** 지정된 고정 상점 테이블로부터 상점 인벤토리를 생성합니다. */
    UFUNCTION(BlueprintCallable, Category = "Shop|Generation")
    FERNShopInventory GenerateFixedInventory(FName ShopID, EShopType ShopType, class UDataTable* FixedDataTable);

    // ===== 델리게이트 선언부 (Provider 구현체에는 필수) =====
    UPROPERTY(BlueprintAssignable, Category = "Shop")
    FOnShopDataReceived OnShopDataReceived;

    UPROPERTY(BlueprintAssignable, Category = "Shop")
    FOnPurchaseComplete OnPurchaseComplete;

    UPROPERTY(BlueprintAssignable, Category = "Shop")
    FOnShopError OnShopError;

protected:
    // 상점 전용 상품 목록 테이블 (FERNShopProductTable 구조체 사용)
    UPROPERTY(EditDefaultsOnly, Category = "Shop Data")
    UDataTable* ShopProductTable;

    UPROPERTY()
    UObject* ShopDataProvider = nullptr;
    
    void InitalizeShopSysttem();
    UPROPERTY(EditDefaultsOnly, Category ="Shop")
    TSubclassOf<UERNDataTableShopProvider> DataTableProviderClass;
    
    /** 랜덤 시드 (0 = 완전 랜덤, 양수 = 시드 고정) — 디버깅용 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop Debug")
    int32 DebugRandomSeed = 0;

public:
    // 상인 ID별로 생성된 인벤토리를 캐싱하는 맵
    UPROPERTY()
    TMap<FName, FERNShopInventory> CachedShopData;

protected:
    
    // 데이터가 캐싱되어 준비되었는지 여부 
    bool bIsDataCached = false;
    
    // 소유자 참조
    UPROPERTY()
    UObject* OwnerObject = nullptr;
};
