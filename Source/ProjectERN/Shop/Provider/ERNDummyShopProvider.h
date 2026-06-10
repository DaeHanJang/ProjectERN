// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Shop/Data/ERNShopTypes.h"
#include "Shop/Provider/ERNShopDataProvider.h"
#include "ERNDummyShopProvider.generated.h"



/**
 * UERNDummyShopProvider - 더미 데이터 기반 상점 Provider
 * 
 * 네트워크/DB 없이 하드코딩된 더미 데이터를 즉시 반환합니다.
 * 개발 초기 단계에서 상점 로직을 테스트하는 데 사용합니다.
 */
UCLASS(BlueprintType)
class PROJECTERN_API UERNDummyShopProvider : public UObject, public IERNShopDataProvider
{
    GENERATED_BODY()

public:

    // ===== IERNShopDataProvider 구현 =====

    virtual void Initialize_Implementation(UObject* Owner) override;
    virtual void RequestShopData_Implementation(EShopType ShopType) override;
    virtual void RequestPurchase_Implementation(FERNShopTransaction Transaction) override;
    virtual FERNShopInventory GetCachedShopData_Implementation(EShopType ShopType) override;
    virtual void ClearCache_Implementation() override;
    virtual bool IsDataReady_Implementation() override;
    virtual void HandleReceivedData_Implementation(const FERNShopInventory& ShopData) override;
    virtual void HandlePurchaseResult_Implementation(const FERNShopTransaction& Transaction) override;

    // ===== 델리게이트 (구현체에서 직접 선언) =====

    UPROPERTY(BlueprintAssignable, Category = "Shop")
    FOnShopDataReceived OnShopDataReceived;

    UPROPERTY(BlueprintAssignable, Category = "Shop")
    FOnPurchaseComplete OnPurchaseComplete;

    UPROPERTY(BlueprintAssignable, Category = "Shop")
    FOnShopError OnShopError;

protected:

    // 캐시 - ShopType별 상점 인벤토리
    UPROPERTY()
    TMap<EShopType, FERNShopInventory> CachedData;

    // 소유자 참조
    UPROPERTY()
    UObject* OwnerObject = nullptr;

    // 더미 아이템 생성
    void GenerateDummyItems(EShopType ShopType);
};
