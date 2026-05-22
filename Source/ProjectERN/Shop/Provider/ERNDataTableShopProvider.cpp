#include "Shop/Provider/ERNDataTableShopProvider.h"
#include "Inventory/Item/Data/ERNItemTable.h"
#include "Inventory/Item/Data/ItemDataAssetBase.h"
#include "Character/Player/ProjectERNCharacter.h"
#include "Inventory/Components/ERNInventoryComponent.h"
#include "GAS/ERNAttributeSet.h"
#include "Inventory/Data/ERNInventoryList.h"
#include "Inventory/Item/Data/ERNItemRuntimeState.h"
#include "Inventory/Item/Manager/ItemManagerSubsystem.h"

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
        ShopItem.UniqueID = FGuid::NewGuid(); // 고유 ID 부여
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
    // 트랜잭션 처리 대상 캐릭터 확인
    AProjectERNCharacter* PlayerChar = Cast<AProjectERNCharacter>(Transaction.Buyer);
    if (!PlayerChar)
    {
        Transaction.Result = EERNTransactionResult::InvalidItem;
        OnPurchaseComplete.Broadcast(Transaction);
        return;
    }

    // 1. 아이템 검색 및 가격/재고 검증
    if (bIsDataCached && CachedShopData.Contains(Transaction.ShopType))
    {
        FERNShopInventory& Inventory = CachedShopData[Transaction.ShopType];
        FERNShopItemData* TargetItem = nullptr;

        for (FERNShopItemData& Item : Inventory.Items)
        {
            if (Item.UniqueID == Transaction.UniqueID)
            {
                TargetItem = &Item;
                break;
            }
        }

        if (!TargetItem)
        {
            Transaction.Result = EERNTransactionResult::InvalidItem;
            OnPurchaseComplete.Broadcast(Transaction);
            return;
        }

        // 재고 검사
        if (TargetItem->StockCount != -1 && TargetItem->StockCount < Transaction.Quantity)
        {
            Transaction.Result = EERNTransactionResult::OutOfStock;
            OnPurchaseComplete.Broadcast(Transaction);
            return;
        }

        // 2. 골드 검사 및 차감
        int32 TotalPrice = TargetItem->Price * Transaction.Quantity;
        Transaction.TotalPrice = TotalPrice;
        
        UERNAttributeSet* AttributeSet = PlayerChar->GetAttributeSet();
        if (!AttributeSet || AttributeSet->GetGold() < TotalPrice)
        {
            Transaction.Result = EERNTransactionResult::InsufficientFunds;
            OnPurchaseComplete.Broadcast(Transaction);
            return;
        }

        // 골드 차감
        AttributeSet->SetGold(AttributeSet->GetGold() - TotalPrice);

        // 3. 인벤토리 추가
        UERNInventoryComponent* InvComp = PlayerChar->GetInventoryComponent();
        if (!InvComp)
        {
            // 인벤토리 컴포넌트가 없다면 골드 롤백 후 실패 처리
            AttributeSet->SetGold(AttributeSet->GetGold() + TotalPrice);
            Transaction.Result = EERNTransactionResult::InvalidItem;
            OnPurchaseComplete.Broadcast(Transaction);
            return;
        }

        FItemRuntimeState NewItemState;
        NewItemState.SetItemID(TargetItem->ItemID);
        NewItemState.SetQuantity(Transaction.Quantity);

        TArray<FInventoryItemEntry> ChangedEntries;
        
        // 아이템의 실제 최대 중첩(Stack) 사이즈를 데이터 테이블에서 조회
        int32 ActualMaxStackSize = 99; // 기본값 (안전 장치)
        if (UGameInstance* GI = PlayerChar->GetGameInstance())
        {
            if (UItemManagerSubsystem* ItemMgr = GI->GetSubsystem<UItemManagerSubsystem>())
            {
                if (const FERNItemTable* ItemRow = ItemMgr->FindItemRow(TargetItem->ItemID))
                {
                    ActualMaxStackSize = ItemRow->MaxStackSize;
                }
            }
        }

        bool bAddSuccess = InvComp->GetInventory().AddItem(NewItemState, InvComp->GetMaxSlotSize(), ActualMaxStackSize, ChangedEntries);

        if (!bAddSuccess)
        {
            // 롤백: 골드 복구
            AttributeSet->SetGold(AttributeSet->GetGold() + TotalPrice);
            Transaction.Result = EERNTransactionResult::InventoryFull;
            OnPurchaseComplete.Broadcast(Transaction);
            return;
        }

        // 인벤토리 UI 갱신 (변경된 슬롯마다 델리게이트 브로드캐스트)
        for (const FInventoryItemEntry& Entry : ChangedEntries)
        {
            InvComp->OnInventorySlotChanged.Broadcast(Entry);
        }

        // 4. 재고 차감 (무제한이 아닐 경우)
        if (TargetItem->StockCount != -1)
        {
            TargetItem->StockCount -= Transaction.Quantity;
            if (TargetItem->StockCount <= 0)
            {
                TargetItem->bIsAvailable = false;
            }
        }

        // 트랜잭션에 원래 ItemID 기록 (UI 등에서 식별/출력용)
        Transaction.ItemID = TargetItem->ItemID;

        // 5. 트랜잭션 성공 처리
        Transaction.Result = EERNTransactionResult::Success;

        // 다른 플레이어들에게도 상점 데이터 변경 알림 (멀티플레이어 재고 동기화)
        // 브로드캐스트는 서버 내의 모든 UERNShopComponent의 OnDataReceived를 트리거
        OnShopDataReceived.Broadcast(Inventory);
    }
    else
    {
        Transaction.Result = EERNTransactionResult::InvalidItem;
    }

    // 6. 구매 결과 반환 (구매자에게만 적용되도록 컴포넌트에서 필터링됨)
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
