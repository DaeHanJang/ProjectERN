// Copyright Epic Games, Inc. All Rights Reserved.

#include "Shop/Provider/ERNNetworkShopProvider.h"

void UERNNetworkShopProvider::Initialize_Implementation(UObject* Owner)
{
    OwnerObject = Owner;
    UE_LOG(LogShopProvider, Log, TEXT("[NetworkProvider] 초기화 완료. Owner: %s"),
        Owner ? *Owner->GetName() : TEXT("nullptr"));
}

void UERNNetworkShopProvider::RequestShopData_Implementation(FName ShopID)
{
    UE_LOG(LogShopProvider, Log, TEXT("[NetworkProvider] 상점 데이터 요청: %s"), *ShopID.ToString());

    // 캐시 히트 + 유효 → 즉시 반환
    if (IsCacheValid(ShopID))
    {
        UE_LOG(LogShopProvider, Log, TEXT("[NetworkProvider] 캐시 히트 → 즉시 반환 (아이템 수: %d)"),
            CachedData[ShopID].Items.Num());
        OnShopDataReceived.Broadcast(CachedData[ShopID]);
        return;
    }

    // 이미 요청 중이면 중복 요청 방지
    if (PendingRequests.Contains(ShopID))
    {
        UE_LOG(LogShopProvider, Log, TEXT("[NetworkProvider] 이미 요청 중: %s (중복 요청 무시)"),
            *ShopID.ToString());
        return;
    }

    // 캐시 미스 → ShopComponent가 Server RPC로 요청해야 함
    // 여기서는 단순히 요청 중 상태만 마킹
    PendingRequests.Add(ShopID);
    UE_LOG(LogShopProvider, Log, TEXT("[NetworkProvider] 캐시 미스 → RPC 필요 (ShopComponent가 처리)"));

    // 에러 이벤트 발생 (ShopComponent가 이를 감지하고 RPC 발송)
    OnShopError.Broadcast(FString::Printf(TEXT("CACHE_MISS:%s"), *ShopID.ToString()));
}

void UERNNetworkShopProvider::RequestPurchase_Implementation(FERNShopTransaction Transaction)
{
    UE_LOG(LogShopProvider, Log, TEXT("[NetworkProvider] 구매 요청: %s x%d"),
        *Transaction.ItemID.ToString(), Transaction.Quantity);

    // 로컬 사전 검증 (UI 응답성 향상)
    bool bLocalValid = false;
    for (auto& Pair : CachedData)
    {
        for (const FERNShopItemData& Item : Pair.Value.Items)
        {
            if (Item.ItemID == Transaction.ItemID)
            {
                if (!Item.bIsAvailable)
                {
                    Transaction.Result = EERNTransactionResult::InvalidItem;
                    OnPurchaseComplete.Broadcast(Transaction);
                    return;
                }
                if (Item.StockCount != -1 && Item.StockCount < Transaction.Quantity)
                {
                    Transaction.Result = EERNTransactionResult::OutOfStock;
                    OnPurchaseComplete.Broadcast(Transaction);
                    return;
                }
                bLocalValid = true;
                break;
            }
        }
        if (bLocalValid) break;
    }

    // 로컬 검증 통과 → 서버 검증은 ShopComponent가 Server RPC로 처리
    // 여기서는 로컬 검증 실패 시만 즉시 반환
    if (!bLocalValid)
    {
        UE_LOG(LogShopProvider, Log,
            TEXT("[NetworkProvider] 로컬 검증 통과 → 서버 RPC 대기"));
    }
}

FERNShopInventory UERNNetworkShopProvider::GetCachedShopData_Implementation(FName ShopID)
{
    if (CachedData.Contains(ShopID))
    {
        return CachedData[ShopID];
    }
    return FERNShopInventory();
}

bool UERNNetworkShopProvider::IsDataReady_Implementation()
{
    return CachedData.Num() > 0;
}

void UERNNetworkShopProvider::HandleReceivedData_Implementation(const FERNShopInventory& ShopData)
{
    UE_LOG(LogShopProvider, Log, TEXT("[NetworkProvider] 데이터 수신 → 캐시 저장 (ShopID: %s, 아이템: %d)"),
        *ShopData.ShopID.ToString(), ShopData.Items.Num());

    // 캐시에 저장
    CachedData.Add(ShopData.ShopID, ShopData);

    // 타임스탬프 갱신
    float CurrentTime = OwnerObject && OwnerObject->GetWorld()
        ? OwnerObject->GetWorld()->GetTimeSeconds()
        : 0.f;
    CacheTimestamps.Add(ShopData.ShopID, CurrentTime);

    // 요청 대기 상태 해제
    PendingRequests.Remove(ShopData.ShopID);

    // 이벤트 발생
    OnShopDataReceived.Broadcast(ShopData);
}

void UERNNetworkShopProvider::HandlePurchaseResult_Implementation(const FERNShopTransaction& Transaction)
{
    UE_LOG(LogShopProvider, Log, TEXT("[NetworkProvider] 구매 결과 수신: %s (결과: %d)"),
        *Transaction.ItemID.ToString(), static_cast<int32>(Transaction.Result));

    // 구매 성공 시 로컬 캐시에서 재고 차감
    if (Transaction.Result == EERNTransactionResult::Success)
    {
        for (auto& Pair : CachedData)
        {
            for (FERNShopItemData& Item : Pair.Value.Items)
            {
                if (Item.ItemID == Transaction.ItemID && Item.StockCount != -1)
                {
                    Item.StockCount = FMath::Max(0, Item.StockCount - Transaction.Quantity);
                    break;
                }
            }
        }
    }

    OnPurchaseComplete.Broadcast(Transaction);
}

// ===== 캐시 관리 =====

void UERNNetworkShopProvider::InvalidateCache(FName ShopID)
{
    CachedData.Remove(ShopID);
    CacheTimestamps.Remove(ShopID);
    UE_LOG(LogShopProvider, Log, TEXT("[NetworkProvider] 캐시 무효화: %s"), *ShopID.ToString());
}

void UERNNetworkShopProvider::ClearAllCache()
{
    CachedData.Empty();
    CacheTimestamps.Empty();
    PendingRequests.Empty();
    UE_LOG(LogShopProvider, Log, TEXT("[NetworkProvider] 전체 캐시 초기화"));
}

bool UERNNetworkShopProvider::IsCacheValid(FName ShopID) const
{
    if (!CachedData.Contains(ShopID) || !CacheTimestamps.Contains(ShopID))
    {
        return false;
    }

    float CurrentTime = OwnerObject && OwnerObject->GetWorld()
        ? OwnerObject->GetWorld()->GetTimeSeconds()
        : 0.f;
    float CachedTime = CacheTimestamps[ShopID];

    return (CurrentTime - CachedTime) < CacheTTL;
}
