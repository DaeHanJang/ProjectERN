// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Shop/Data/ERNShopTypes.h"
#include "Inventory/Item/Data/ERNItemEnums.h"
#include "ERNShopDataProvider.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UERNShopDataProvider : public UInterface
{
    GENERATED_BODY()
};

/**
 * IERNShopDataProvider - 상점 데이터 제공자 인터페이스
 * 
 * 모든 데이터 Provider(Dummy, Network 등)는 이 인터페이스를 구현합니다.
 * Provider는 UObject 기반이므로 RPC를 직접 사용할 수 없습니다.
 * 네트워크 통신은 ShopComponent(ActorComponent)가 브릿지 역할을 합니다.
 */
class IERNShopDataProvider
{
    GENERATED_BODY()

public:

    // ===== 초기화 =====

    /**
     * Provider 초기화
     * @param Owner - 이 Provider를 소유한 객체 (보통 GameInstance)
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Shop|Provider")
    void Initialize(UObject* Owner);

    // ===== 데이터 요청 =====

    /**
     * 상점 데이터 요청
     * 결과는 OnShopDataReceived 델리게이트로 전달
     * @param ShopID - 요청할 상점 ID
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Shop|Provider")
    void RequestShopData(EShopType ShopType);

    /**
     * 구매 요청
     * 결과는 OnPurchaseComplete 델리게이트로 전달
     * @param Transaction - 거래 정보
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Shop|Provider")
    void RequestPurchase(FERNShopTransaction Transaction);

    // ===== 캐시 조회 =====

    /**
     * 캐시된 상점 데이터 반환
     * @param ShopType - 조회할 상점 타입
     * @return 캐시된 FERNShopInventory (캐시 미스 시 빈 구조체)
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Shop|Provider")
    FERNShopInventory GetCachedShopData(EShopType ShopType);

    /**
     * 데이터 준비 여부 확인
     * @return 캐시에 유효한 데이터가 있으면 true
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Shop|Provider")
    bool IsDataReady();

    // ===== 네트워크 Provider 전용 =====

    /**
     * 외부에서 수신된 데이터를 Provider에 주입 (ShopComponent가 Client RPC로 받은 데이터 전달)
     * @param ShopData - 수신된 상점 데이터
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Shop|Provider")
    void HandleReceivedData(const FERNShopInventory& ShopData);

    /**
     * 외부에서 수신된 구매 결과를 Provider에 주입
     * @param Transaction - 구매 결과
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Shop|Provider")
    void HandlePurchaseResult(const FERNShopTransaction& Transaction);
};
