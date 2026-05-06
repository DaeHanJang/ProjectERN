#include "Shop/Provider/ERNDataTableShopProvider.h"
#include "Inventory/Item/Data/ERNItemTable.h"
#include "Inventory/Item/Data/ItemDataAssetBase.h"

UERNDataTableShopProvider::UERNDataTableShopProvider()
{
}

void UERNDataTableShopProvider::Initialize_Implementation(UObject* Owner)
{
    OwnerObject = Owner;
    
    if (!ItemDataTable)
    {
        UE_LOG(LogShopProvider, Error, TEXT("[DataTableProvider] ItemDataTable이 할당되지 않아 캐싱을 진행할 수 없습니다!"));
        return;
    }
    
    // 데이터 테이블의 모든 Row 가져오기
    TArray<FERNItemTable*> AllItems;
    ItemDataTable->GetAllRows<FERNItemTable>(TEXT("ShopProviderCache"), AllItems);
    
    // RowMap을 순회하며 아이템을 상점 타입별로 캐싱
    for (auto& Pair : ItemDataTable->GetRowMap())
    {
        FName ItemID = Pair.Key;
        FERNItemTable* RowData = reinterpret_cast<FERNItemTable*>(Pair.Value);

        if (!RowData) continue;

        // 아이템 가격표를 순회하며 판매되는 상점 타입을 확인.
        for (const FItemShopPrice& PriceData : RowData->ShopPrices)
        {
            if (PriceData.ShopType == EShopType::None) continue;
            
            // 해당 상점 타입의 인벤토리가 맵에 없을 경우 새로 생성
            if (!CachedShopData.Contains(PriceData.ShopType))
            {
                FERNShopInventory NewInventory;
                NewInventory.ShopType = PriceData.ShopType;
                CachedShopData.Add(PriceData.ShopType, NewInventory);
            }
            
            //상점 아이템 데이터 구성
            FERNShopItemData ShopItem;
            ShopItem.ItemID = ItemID;
            ShopItem.Price = PriceData.BuyPrice;
            ShopItem.StockCount = -1;
            ShopItem.bIsAvailable = true;
            // 캐싱 맵의 해당 상점 타입 인벤토리 아이템 추가
            CachedShopData[PriceData.ShopType].Items.Add(ShopItem);
        }
    }

    bIsDataCached = true;
    UE_LOG(LogShopProvider, Log, TEXT("[DataTableProvider] 초기화 및 데이터 테이블 캐싱 완료. 총 %d종류의 상점 데이터가 준비되었습니다."), CachedShopData.Num());
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
