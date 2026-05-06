// Copyright Epic Games, Inc. All Rights Reserved.

#include "Shop/Provider/ERNDummyShopProvider.h"



void UERNDummyShopProvider::Initialize_Implementation(UObject* Owner)
{
    OwnerObject = Owner;
    UE_LOG(LogShopProvider, Log, TEXT("[DummyProvider] 초기화 완료. Owner: %s"),
        Owner ? *Owner->GetName() : TEXT("nullptr"));
}

void UERNDummyShopProvider::RequestShopData_Implementation(EShopType ShopType)
{
    UE_LOG(LogShopProvider, Log, TEXT("[DummyProvider] 상점 데이터 요청: %d"), (int32)ShopType);

    // 캐시에 없으면 더미 데이터 생성
    if (!CachedData.Contains(ShopType))
    {
        GenerateDummyItems(ShopType);
    }

    // 즉시 이벤트 발생 (네트워크 지연 없음)
    if (CachedData.Contains(ShopType))
    {
        OnShopDataReceived.Broadcast(CachedData[ShopType]);
        UE_LOG(LogShopProvider, Log, TEXT("[DummyProvider] 데이터 전달 완료. 아이템 수: %d"),
            CachedData[ShopType].Items.Num());
    }
}

void UERNDummyShopProvider::RequestPurchase_Implementation(FERNShopTransaction Transaction)
{
    UE_LOG(LogShopProvider, Log, TEXT("[DummyProvider] 구매 요청: %s x%d"),
        *Transaction.ItemID.ToString(), Transaction.Quantity);

    // 상점 및 아이템 검색 (위임 패턴으로 전달받은 ShopID 기반)
    FERNShopInventory* ShopInv = nullptr;
    FERNShopItemData* FoundItem = nullptr;

    if (CachedData.Contains(Transaction.ShopType))
    {
        ShopInv = &CachedData[Transaction.ShopType];
        for (FERNShopItemData& Item : ShopInv->Items)
        {
            if (Item.ItemID == Transaction.ItemID)
            {
                FoundItem = &Item;
                break;
            }
        }
    }

    // 검증
    if (!FoundItem)
    {
        Transaction.Result = EERNTransactionResult::InvalidItem;
        UE_LOG(LogShopProvider, Warning, TEXT("[DummyProvider] 구매 실패 - 아이템 없음: %s"),
            *Transaction.ItemID.ToString());
    }
    else if (!FoundItem->bIsAvailable)
    {
        Transaction.Result = EERNTransactionResult::InvalidItem;
        UE_LOG(LogShopProvider, Warning, TEXT("[DummyProvider] 구매 실패 - 비매품"));
    }
    else if (FoundItem->StockCount != -1 && FoundItem->StockCount < Transaction.Quantity)
    {
        Transaction.Result = EERNTransactionResult::OutOfStock;
        UE_LOG(LogShopProvider, Warning, TEXT("[DummyProvider] 구매 실패 - 재고 부족 (남은 수량: %d)"),
            FoundItem->StockCount);
    }
    else
    {
        // 구매 성공
        Transaction.Result = EERNTransactionResult::Success;
        Transaction.TotalPrice = FoundItem->Price * Transaction.Quantity;

        // 재고 차감 (무제한이 아닌 경우)
        if (FoundItem->StockCount != -1)
        {
            FoundItem->StockCount -= Transaction.Quantity;
        }

        UE_LOG(LogShopProvider, Log, TEXT("[DummyProvider] 구매 성공: %s x%d, 총 가격: %d"),
            *Transaction.ItemID.ToString(), Transaction.Quantity, Transaction.TotalPrice);
    }

    Transaction.Timestamp = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;
    OnPurchaseComplete.Broadcast(Transaction);
}

FERNShopInventory UERNDummyShopProvider::GetCachedShopData_Implementation(EShopType ShopType)
{
    if (CachedData.Contains(ShopType))
    {
        return CachedData[ShopType];
    }
    return FERNShopInventory();
}

bool UERNDummyShopProvider::IsDataReady_Implementation()
{
    return CachedData.Num() > 0;
}

void UERNDummyShopProvider::HandleReceivedData_Implementation(const FERNShopInventory& ShopData)
{
    // DummyProvider에서는 사용하지 않음 (네트워크 수신용)
    UE_LOG(LogShopProvider, Log, TEXT("[DummyProvider] HandleReceivedData 호출됨 (무시)"));
}

void UERNDummyShopProvider::HandlePurchaseResult_Implementation(const FERNShopTransaction& Transaction)
{
    // DummyProvider에서는 사용하지 않음 (네트워크 수신용)
    UE_LOG(LogShopProvider, Log, TEXT("[DummyProvider] HandlePurchaseResult 호출됨 (무시)"));
}

void UERNDummyShopProvider::GenerateDummyItems(EShopType ShopType)
{
    FERNShopInventory Inventory;
    Inventory.ShopType = ShopType;
    Inventory.ShopLevel = 1;

    Inventory.bIsSharedPool = true;

    // ----- 공통 기본 아이템 (야영지 등 모든 상점 취급) -----
    // 무기
    {
        FERNShopItemData Item;
        Item.ItemID = FName("ITEM_SWORD_01");
        Item.Price = 500;  // 500 룬
        Item.StockCount = 1;  // 공유 재고: 1개
        Item.bIsAvailable = true;
        Inventory.Items.Add(Item);
    }

    // (방어구 카테고리 제거됨)

    // 소모품
    {
        FERNShopItemData Item;
        Item.ItemID = FName("ITEM_POTION_HP_01");
        Item.Price = 50;  // 50 룬
        Item.StockCount = -1;  // 무제한
        Item.bIsAvailable = true;
        Inventory.Items.Add(Item);
    }
    {
        FERNShopItemData Item;
        Item.ItemID = FName("ITEM_POTION_MP_01");
        Item.Price = 80;  // 80 룬
        Item.StockCount = -1;
        Item.bIsAvailable = true;
        Inventory.Items.Add(Item);
    }

    // ----- 마을 상점 (Shop_Township) 이상 등급 전용 확장 아이템 -----
    if (ShopType == EShopType::World || ShopType == EShopType::Boss)
    {
        FERNShopItemData Item;
        Item.ItemID = FName("ITEM_STAFF_01");
        Item.Price = 800;
        Item.StockCount = 1;
        Item.bIsAvailable = true;
        Inventory.Items.Add(Item);


    }

    // ----- 보스방 특수 상점 (Shop_Boss) 전용 아이템 -----
    if (ShopType == EShopType::Boss)
    {
        FERNShopItemData Item;
        Item.ItemID = FName("ITEM_POTION_SPECIAL");
        Item.Price = 3000;
        Item.StockCount = 3; // 한정 판매
        Item.bIsAvailable = true;
        Inventory.Items.Add(Item);
    }

    CachedData.Add(ShopType, Inventory);

    UE_LOG(LogShopProvider, Log, TEXT("[DummyProvider] 더미 아이템 %d개 생성 (ShopType: %d)"),
        Inventory.Items.Num(), (int32)ShopType);
}
