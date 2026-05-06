#include "Shop/Provider/ERNDataTableShopProvider.h"
#include "Inventory/Item/Data/ERNItemTable.h"
#include "Inventory/Item/Data/ItemDataAssetBase.h"

UERNDataTableShopProvider::UERNDataTableShopProvider()
{
}

void UERNDataTableShopProvider::Initialize_Implementation(UObject* Owner)
{
    OwnerObject = Owner;
    UE_LOG(LogShopProvider, Log, TEXT("[DataTableProvider] 초기화 완료."));
}

void UERNDataTableShopProvider::RequestShopData_Implementation(EShopType ShopType)
{
    FERNShopInventory ShopInventory;
    ShopInventory.ShopType = ShopType;
    
    if (!ItemDataTable)
    {
        UE_LOG(LogShopProvider, Error, TEXT("[DataTableProvider] ItemDataTable이 할당되지 않았습니다!"));
        OnShopDataReceived.Broadcast(ShopInventory);
        return;
    }

    // 페이즈 3에서 여기에 데이터 추출 로직 작성 예정

    OnShopDataReceived.Broadcast(ShopInventory);
}

void UERNDataTableShopProvider::RequestPurchase_Implementation(FERNShopTransaction Transaction)
{
    // 페이즈 3 또는 별도 구현 예정
    Transaction.Result = EERNTransactionResult::InvalidItem;
    OnPurchaseComplete.Broadcast(Transaction);
}

FERNShopInventory UERNDataTableShopProvider::GetCachedShopData_Implementation(EShopType ShopType)
{
    return FERNShopInventory();
}

bool UERNDataTableShopProvider::IsDataReady_Implementation()
{
    return true;
}

void UERNDataTableShopProvider::HandleReceivedData_Implementation(const FERNShopInventory& ShopData)
{
}

void UERNDataTableShopProvider::HandlePurchaseResult_Implementation(const FERNShopTransaction& Transaction)
{
}
