// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Shop/Data/ERNShopTypes.h"
#include "Shop/Provider/ERNShopDataProvider.h"
#include "ERNNetworkShopProvider.generated.h"

/**
 * UERNNetworkShopProvider - 네트워크 기반 상점 Provider
 * 
 * 역할:
 * - 로컬 캐시 관리 (캐시 히트 시 RPC 없이 즉시 반환)
 * - ShopComponent의 Client RPC로 수신된 데이터를 캐시에 저장
 * - 캐시 TTL(Time To Live) 관리
 * 
 * 주의: UObject이므로 RPC를 직접 사용할 수 없습니다.
 * 모든 네트워크 통신은 ShopComponent(ActorComponent)가 담당합니다.
 */
UCLASS(BlueprintType)
class PROJECTERN_API UERNNetworkShopProvider : public UObject, public IERNShopDataProvider
{
    GENERATED_BODY()

public:

    // ===== IERNShopDataProvider 구현 =====

    virtual void Initialize_Implementation(UObject* Owner) override;
    virtual void RequestShopData_Implementation(EShopType ShopType) override;
    virtual void RequestPurchase_Implementation(FERNShopTransaction Transaction) override;
    virtual FERNShopInventory GetCachedShopData_Implementation(EShopType ShopType) override;
    virtual bool IsDataReady_Implementation() override;
    virtual void HandleReceivedData_Implementation(const FERNShopInventory& ShopData) override;
    virtual void HandlePurchaseResult_Implementation(const FERNShopTransaction& Transaction) override;

    // ===== 델리게이트 =====

    UPROPERTY(BlueprintAssignable, Category = "Shop")
    FOnShopDataReceived OnShopDataReceived;

    UPROPERTY(BlueprintAssignable, Category = "Shop")
    FOnPurchaseComplete OnPurchaseComplete;

    UPROPERTY(BlueprintAssignable, Category = "Shop")
    FOnShopError OnShopError;

    // ===== 캐시 관리 =====

    /** 특정 상점의 캐시 무효화 */
    UFUNCTION(BlueprintCallable, Category = "Shop|Cache")
    void InvalidateCache(EShopType ShopType);

    /** 전체 캐시 초기화 */
    UFUNCTION(BlueprintCallable, Category = "Shop|Cache")
    void ClearAllCache();

    /** 캐시가 유효한지 확인 (TTL 기반) */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Shop|Cache")
    bool IsCacheValid(EShopType ShopType) const;

protected:

    // 캐시 - ShopType별 상점 인벤토리
    UPROPERTY()
    TMap<EShopType, FERNShopInventory> CachedData;

    // 캐시 타임스탬프 - ShopType별 마지막 갱신 시각
    TMap<EShopType, float> CacheTimestamps;

    // 캐시 유효 시간 (초)
    UPROPERTY(EditAnywhere, Category = "Shop|Cache")
    float CacheTTL = 60.f;

    // 소유자 참조
    UPROPERTY()
    UObject* OwnerObject = nullptr;

    // 데이터 요청 중 여부 (중복 요청 방지)
    TSet<EShopType> PendingRequests;
};
