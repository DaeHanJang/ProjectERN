// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Shop/Data/ERNShopTypes.h"
#include "Inventory/Item/Data/ERNItemEnums.h"
#include "ERNShopComponent.generated.h"

class IERNShopDataProvider;

/**
 * UERNShopComponent - 상점 Consumer + 네트워크 브릿지
 * 
 * 역할:
 * 1. Consumer: Provider에 데이터를 요청하고 결과를 받아 UI에 전달
 * 2. 네트워크 브릿지: Server/Client RPC를 통한 호스트-클라이언트 통신
 * 
 * RPC는 AActor/UActorComponent에서만 사용 가능하므로,
 * Provider(UObject) 대신 이 컴포넌트가 RPC를 담당합니다.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTERN_API UERNShopComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UERNShopComponent();

    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // ===== 공개 API =====

public:
    /** 상점 열기 */
    UFUNCTION(BlueprintCallable, Category = "Shop")
    void OpenShop(EShopType ShopType, AActor* TargetNPC = nullptr);

    /** 마스터 테이블 방식 상점 열기 (페이즈 4) */
    UFUNCTION(BlueprintCallable, Category = "Shop")
    void OpenShopRandom(FName RequestShopID, EShopType ShopType, const TArray<FERNShopSlotConfig>& SlotConfigs, AActor* TargetNPC = nullptr);

    /** 고정 데이터 테이블 방식 상점 열기 */
    UFUNCTION(BlueprintCallable, Category = "Shop")
    void OpenShopFixed(FName RequestShopID, EShopType ShopType, const TArray<FERNShopSlotConfig>& SlotConfigs, class UDataTable* FixedDataTable, AActor* TargetNPC = nullptr);

    /** 상점 닫기 */
    UFUNCTION(BlueprintCallable, Category = "Shop")
    void CloseShop();

    /** 아이템 구매 시도 */
    UFUNCTION(BlueprintCallable, Category = "Shop")
    void TryPurchaseItem(FGuid UniqueID, int32 Quantity = 1);

    /** 현재 상점 아이템 목록 반환 */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Shop")
    TArray<FERNShopItemData> GetCurrentShopItems() const;

    /** 상점이 열려 있는지 여부 */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Shop")
    bool IsShopOpen() const { return bIsShopOpen; }

    // ===== UI 갱신 이벤트 =====

    UPROPERTY(BlueprintAssignable, Category = "Shop")
    FOnShopUIUpdate OnShopUIUpdate;

    UPROPERTY(BlueprintAssignable, Category = "Shop")
    FOnPurchaseComplete OnPurchaseResult;

protected:

    // ===== Server RPC (위임 패턴) =====

    /** 클라이언트 → 서버: 상점 데이터 요청 */
    UFUNCTION(Server, Reliable, Category = "Shop")
    void Server_RequestShopData(EShopType ShopType, AActor* TargetNPC);

    /** 클라이언트 → 서버: 단일 마스터 테이블 기반 상점 생성 및 열기 요청 */
    UFUNCTION(Server, Reliable, Category = "Shop")
    void Server_OpenShopRandom(FName RequestShopID, EShopType ShopType, const TArray<FERNShopSlotConfig>& SlotConfigs, AActor* TargetNPC);

    /** 클라이언트 → 서버: 고정 상점 데이터 기반 상점 열기 요청 */
    UFUNCTION(Server, Reliable, Category = "Shop")
    void Server_OpenShopFixed(FName RequestShopID, EShopType ShopType, const TArray<FERNShopSlotConfig>& SlotConfigs, class UDataTable* FixedDataTable, AActor* TargetNPC);

    /** 클라이언트 → 서버: 구매 요청 (안전한 파라미터) */
    UFUNCTION(Server, Reliable, Category = "Shop")
    void Server_RequestPurchase(FGuid UniqueID, int32 Quantity, AActor* TargetNPC);

    // ===== Client RPC =====

    /** 서버 → 클라이언트: 상점 데이터 전달 */
    UFUNCTION(Client, Reliable, Category = "Shop")
    void Client_ReceiveShopData(FERNShopInventory ShopData);

    /** 서버 → 클라이언트: 구매 결과 전달 */
    UFUNCTION(Client, Reliable, Category = "Shop")
    void Client_PurchaseResult(const FERNShopTransaction& Transaction);

    // ===== 내부 콜백 =====

    /** Provider의 OnShopDataReceived 이벤트 수신 */
    UFUNCTION()
    void OnDataReceived(const FERNShopInventory& ShopData);

    /** Provider의 OnPurchaseComplete 이벤트 수신 */
    UFUNCTION()
    void OnPurchaseComplete(const FERNShopTransaction& Transaction);

private:

    // Provider 참조 (GameInstance에서 획득)
    IERNShopDataProvider* DataProvider = nullptr;

    // 현재 열린 상점 타입
    EShopType CurrentShopType = EShopType::None;

    // 현재 열려있는 상점의 ShopID (트랜잭션 결제 시 사용)
    UPROPERTY(BlueprintReadOnly, Category = "Shop", meta = (AllowPrivateAccess = "true"))
    FName CurrentShopID;

    // 로컬 캐시 (수신된 상점 데이터)
    FERNShopInventory CurrentShopData;

    // 상점 열림 상태
    bool bIsShopOpen = false;

    // 상호작용 중인 대상 NPC (서버 검증용)
    UPROPERTY()
    AActor* CurrentTargetNPC = nullptr;

    // Provider 획득 헬퍼
    void AcquireProvider();
};
