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
    
    // 초기화 시 데이터 테이블을 파싱하여 캐싱해둘 맵 (상점 타입별로 인벤토리 분류)
    UPROPERTY()
    TMap<EShopType, FERNShopInventory> CachedShopData;
    
    // 데이터가 캐싱되어 준비되었는지 여부 
    bool bIsDataCached = false;
    
    // 소유자 참조
    UPROPERTY()
    UObject* OwnerObject = nullptr;
};
