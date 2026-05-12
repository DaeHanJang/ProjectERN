#include "Shop/Provider/ERNDataTableShopProvider.h"
#include "Inventory/Item/Data/ERNItemTable.h"
#include "Inventory/Item/Data/ItemDataAssetBase.h"

UERNDataTableShopProvider::UERNDataTableShopProvider()
{
}

void UERNDataTableShopProvider::Initialize_Implementation(UObject* Owner)
{
    OwnerObject = Owner;
    
    if (!ShopProductTable)
    {
        UE_LOG(LogShopProvider, Error, TEXT("[DataTableProvider] ShopProductTable이 할당되지 않아 캐싱을 진행할 수 없습니다!"));
        return;
    }
    
    // 상점 상품 데이터 테이블의 모든 Row 가져오기
    TArray<FERNShopProductTable*> AllProducts;
    ShopProductTable->GetAllRows<FERNShopProductTable>(TEXT("ShopProviderCache"), AllProducts);
    
    // Row를 순회하며 상점 타입별로 아이템 캐싱
    for (FERNShopProductTable* ProductData : AllProducts)
    {
        if (!ProductData || ProductData->ShopType == EShopType::None) continue;
        
        UE_LOG(LogShopProvider, Warning, TEXT("[DataTableProvider] ★ 캐싱 상품: %s (상점: %d)"), *ProductData->ItemID.ToString(), (int32)ProductData->ShopType);

        // 해당 상점 타입의 인벤토리가 맵에 없을 경우 새로 생성
        if (!CachedShopData.Contains(ProductData->ShopType))
        {
            FERNShopInventory NewInventory;
            NewInventory.ShopType = ProductData->ShopType;
            CachedShopData.Add(ProductData->ShopType, NewInventory);
        }
        
        // 상점 아이템 데이터 구성
        FERNShopItemData ShopItem;
        ShopItem.ItemID = ProductData->ItemID;
        ShopItem.Price = ProductData->Price;
        ShopItem.StockCount = ProductData->MaxStock;
        ShopItem.bIsAvailable = (ShopItem.StockCount != 0); // 재고가 0이면 구매 불가

        // 캐싱 맵의 해당 상점 타입 인벤토리에 아이템 추가
        CachedShopData[ProductData->ShopType].Items.Add(ShopItem);
    }

    bIsDataCached = true;
    UE_LOG(LogShopProvider, Log, TEXT("[DataTableProvider] 초기화 및 상점 테이블 캐싱 완료. 총 %d종류의 상점 인벤토리가 준비되었습니다."), CachedShopData.Num());
}

void UERNDataTableShopProvider::RequestShopData_Implementation(EShopType ShopType)
{
    // 캐싱된 맵에 해당 상점 타입의 데이터가 있는지 확인
    if (bIsDataCached && CachedShopData.Contains(ShopType))
    {
        // 캐싱된 데이터를 찾아 즉시 UI로 전달
        const FERNShopInventory& CachedInventory = CachedShopData[ShopType];
        
        UE_LOG(LogShopProvider, Log, TEXT("[DataTableProvider] 상점 타입 [%d] 에 대한 캐싱 데이터 %d개 로드 및 전달 완료!"), 
            (int32)ShopType, CachedInventory.Items.Num());

        OnShopDataReceived.Broadcast(CachedInventory);
    }
    else
    {
        UE_LOG(LogShopProvider, Warning, TEXT("[DataTableProvider] 상점 타입 [%d] 에 대한 캐싱 데이터를 찾을 수 없습니다. (빈 인벤토리 전달)"), (int32)ShopType);
        UE_LOG(LogShopProvider, Warning, TEXT("[DataTableProvider Debug] bIsDataCached = %s, CachedShopData.Num() = %d"), 
            bIsDataCached ? TEXT("True") : TEXT("False"), CachedShopData.Num());
        
        for (auto& Pair : CachedShopData)
        {
            UE_LOG(LogShopProvider, Warning, TEXT("[DataTableProvider Debug] 발견된 캐시 상점 타입: %d"), (int32)Pair.Key);
        }
        
        // 데이터가 없으면 빈 인벤토리로 반환
        FERNShopInventory EmptyInventory;
        EmptyInventory.ShopType = ShopType;
        OnShopDataReceived.Broadcast(EmptyInventory);
    }
}

void UERNDataTableShopProvider::RequestPurchase_Implementation(FERNShopTransaction Transaction)
{
    // 페이즈 3 또는 별도 구현 예정
    Transaction.Result = EERNTransactionResult::InvalidItem;
    OnPurchaseComplete.Broadcast(Transaction);
}

FERNShopInventory UERNDataTableShopProvider::GetCachedShopData_Implementation(EShopType ShopType)
{
    if (bIsDataCached && CachedShopData.Contains(ShopType))
    {
        return CachedShopData[ShopType];
    }
    // 찾지 못경우 빈 인벤토리 반환
    return FERNShopInventory();
}

bool UERNDataTableShopProvider::IsDataReady_Implementation()
{
    return bIsDataCached;
}

void UERNDataTableShopProvider::HandleReceivedData_Implementation(const FERNShopInventory& ShopData)
{
}

void UERNDataTableShopProvider::HandlePurchaseResult_Implementation(const FERNShopTransaction& Transaction)
{
}

void UERNDataTableShopProvider::InitalizeShopSysttem()
{
    // 블루프린트에 할당된 클래스를 기반으로 Provider 객체 생성
    if (DataTableProviderClass)
    {
        ShopDataProvider = NewObject<UObject>(this, DataTableProviderClass);
        UE_LOG(LogShopProvider, Warning, TEXT("[GameInstance] BP_DataTableShopProvider 생성 완료"));
    }
    else
    {
        UE_LOG(LogShopProvider, Warning, TEXT("[GameInstace] DataTableProviderClass가 할당되지 않았습니다! (에디터 BP 연결 필요)"));
        // 애러 로그
    }
    
    // 생성된 Provider 초기화(캐싱 로직 실팽)
    if (ShopDataProvider)
    {
        IERNShopDataProvider* Provider = Cast<IERNShopDataProvider>(ShopDataProvider);
        
        if (Provider)
        {
            IERNShopDataProvider::Execute_Initialize(ShopDataProvider, this);
            UE_LOG(LogShopProvider, Warning, TEXT("[GameInstance] 상점 시스템 초기화 (캐싱) 완료"))
        }
    }
}
