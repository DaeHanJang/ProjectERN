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
    bIsDataCached = true;
    UE_LOG(LogShopProvider, Log, TEXT("[DataTableProvider] 초기화 완료 (단일 마스터 테이블 방식 랜덤 생성 모드 대기)"));
}

FERNShopInventory UERNDataTableShopProvider::GenerateRandomInventory(FName ShopID, EShopType ShopType, const TArray<FERNShopSlotConfig>& SlotConfigs)
{
    FERNShopInventory NewInventory;
    NewInventory.ShopID = ShopID;
    NewInventory.ShopType = ShopType;

    if (!ShopProductTable)
    {
        UE_LOG(LogShopProvider, Warning, TEXT("GenerateRandomInventory: 마스터 테이블(ShopProductTable)이 할당되지 않았습니다."));
        return NewInventory;
    }

    TArray<FERNShopProductTable*> AllEntries;
    ShopProductTable->GetAllRows<FERNShopProductTable>(TEXT("GenerateRandomInventory"), AllEntries);

    FRandomStream RandStream;
    if (DebugRandomSeed > 0) RandStream.Initialize(DebugRandomSeed);
    else RandStream.GenerateNewSeed();

    UItemManagerSubsystem* ItemMgr = nullptr;
    if (OwnerObject && OwnerObject->GetWorld())
    {
        if (UGameInstance* GI = OwnerObject->GetWorld()->GetGameInstance())
        {
            ItemMgr = GI->GetSubsystem<UItemManagerSubsystem>();
        }
    }

    for (const FERNShopSlotConfig& Config : SlotConfigs)
    {
        TArray<FERNShopProductTable> Candidates;
        for (FERNShopProductTable* Entry : AllEntries)
        {
            if (Entry && Entry->ShopType == ShopType)
            {
                EItemType ActualItemType = EItemType::None;
                if (ItemMgr)
                {
                    if (const FERNItemTable* ItemRow = ItemMgr->FindItemRow(Entry->ItemID))
                    {
                        ActualItemType = ItemRow->ItemType;
                    }
                }

                if (ActualItemType == Config.Category)
                {
                    Candidates.Add(*Entry);
                }
            }
        }

        int32 SelectedCount = 0;

        for (auto It = Candidates.CreateIterator(); It; ++It)
        {
            if (It->bGuaranteed && SelectedCount < Config.SlotCount)
            {
                FERNShopItemData ItemData;
                ItemData.ItemID = It->ItemID;
                ItemData.ItemCategory = Config.Category;
                ItemData.StockCount = It->MaxStock;
                
                int32 ActualBuyPrice = 0;
                if (ItemMgr)
                {
                    if (const FERNItemTable* ItemRow = ItemMgr->FindItemRow(ItemData.ItemID))
                    {
                        for (const FItemShopPrice& ShopPrice : ItemRow->ShopPrices)
                        {
                            if (ShopPrice.ShopType == ShopType)
                            {
                                ActualBuyPrice = ShopPrice.BuyPrice;
                                break;
                            }
                        }
                    }
                }
                
                if (It->bIsFree)
                {
                    ActualBuyPrice = 0;
                }
                
                ItemData.Price = ActualBuyPrice;
                ItemData.bIsAvailable = (ItemData.StockCount != 0);
                ItemData.UniqueID = FGuid::NewGuid();

                NewInventory.Items.Add(ItemData);
                It.RemoveCurrent();
                SelectedCount++;
            }
        }

        while (SelectedCount < Config.SlotCount && Candidates.Num() > 0)
        {
            float TotalWeight = 0.f;
            for (const auto& C : Candidates) TotalWeight += C.SpawnWeight;

            if (TotalWeight <= 0.f) break;

            float RandomValue = RandStream.FRandRange(0.f, TotalWeight);
            float Accumulated = 0.f;

            for (int32 i = 0; i < Candidates.Num(); ++i)
            {
                Accumulated += Candidates[i].SpawnWeight;
                if (RandomValue <= Accumulated)
                {
                    FERNShopItemData ItemData;
                    ItemData.ItemID = Candidates[i].ItemID;
                    ItemData.ItemCategory = Config.Category;
                    ItemData.StockCount = Candidates[i].MaxStock;
                    
                    int32 ActualBuyPrice = 0;
                    if (ItemMgr)
                    {
                        if (const FERNItemTable* ItemRow = ItemMgr->FindItemRow(ItemData.ItemID))
                        {
                            for (const FItemShopPrice& ShopPrice : ItemRow->ShopPrices)
                            {
                                if (ShopPrice.ShopType == ShopType)
                                {
                                    ActualBuyPrice = ShopPrice.BuyPrice;
                                    break;
                                }
                            }
                        }
                    }
                    
                    if (Candidates[i].bIsFree)
                    {
                        ActualBuyPrice = 0;
                    }
                    
                    ItemData.Price = ActualBuyPrice;
                    ItemData.bIsAvailable = (ItemData.StockCount != 0);
                    ItemData.UniqueID = FGuid::NewGuid();

                    NewInventory.Items.Add(ItemData);
                    Candidates.RemoveAt(i);
                    SelectedCount++;
                    break;
                }
            }
        }
    }

    // 섞인 순서로 추가된 아이템들을 카테고리(EItemType) 우선순위에 따라 정렬 (장비 -> 소모품 순)
    NewInventory.Items.Sort([](const FERNShopItemData& A, const FERNShopItemData& B)
    {
        return A.ItemCategory < B.ItemCategory;
    });

    UE_LOG(LogShopProvider, Warning, TEXT("====================================="));
    UE_LOG(LogShopProvider, Warning, TEXT("[%s] 랜덤 생성 완료. 총 %d개 아이템 생성됨. (아래는 최종 정렬 결과)"), *ShopID.ToString(), NewInventory.Items.Num());
    
    for (int32 i = 0; i < NewInventory.Items.Num(); ++i)
    {
        const FERNShopItemData& Item = NewInventory.Items[i];
        FString CategoryStr = (Item.ItemCategory == EItemType::Equipable) ? TEXT("장비(Equipable)") : 
                              (Item.ItemCategory == EItemType::Consumable) ? TEXT("소모품(Consumable)") : TEXT("기타");

        UE_LOG(LogShopProvider, Warning, TEXT("  -> [%d] ItemID: %s | 분류: %s"), i, *Item.ItemID.ToString(), *CategoryStr);
    }
    UE_LOG(LogShopProvider, Warning, TEXT("====================================="));

    return NewInventory;
}

FERNShopInventory UERNDataTableShopProvider::GenerateFixedInventory(FName ShopID, EShopType ShopType, UDataTable* FixedDataTable)
{
    FERNShopInventory NewInventory;
    NewInventory.ShopID = ShopID;
    NewInventory.ShopType = ShopType;

    if (!FixedDataTable)
    {
        UE_LOG(LogShopProvider, Warning, TEXT("GenerateFixedInventory: 고정 상점 데이터 테이블이 유효하지 않습니다."));
        return NewInventory;
    }

    TArray<FERNShopProductTable*> AllEntries;
    FixedDataTable->GetAllRows<FERNShopProductTable>(TEXT("GenerateFixedInventory"), AllEntries);

    UItemManagerSubsystem* ItemMgr = nullptr;
    if (OwnerObject && OwnerObject->GetWorld())
    {
        if (UGameInstance* GI = OwnerObject->GetWorld()->GetGameInstance())
        {
            ItemMgr = GI->GetSubsystem<UItemManagerSubsystem>();
        }
    }

    for (FERNShopProductTable* Entry : AllEntries)
    {
        if (Entry)
        {
            FERNShopItemData ItemData;
            ItemData.ItemID = Entry->ItemID;
            ItemData.StockCount = Entry->MaxStock;
            
            int32 ActualBuyPrice = 0;
            if (ItemMgr)
            {
                if (const FERNItemTable* ItemRow = ItemMgr->FindItemRow(ItemData.ItemID))
                {
                    ItemData.ItemCategory = ItemRow->ItemType;
                    for (const FItemShopPrice& ShopPrice : ItemRow->ShopPrices)
                    {
                        if (ShopPrice.ShopType == ShopType)
                        {
                            ActualBuyPrice = ShopPrice.BuyPrice;
                            break;
                        }
                    }
                }
            }
            
            if (Entry->bIsFree)
            {
                ActualBuyPrice = 0;
            }
            
            ItemData.Price = ActualBuyPrice;
            ItemData.bIsAvailable = (ItemData.StockCount != 0);
            ItemData.UniqueID = FGuid::NewGuid();

            NewInventory.Items.Add(ItemData);
        }
    }

    // 아이템 정렬 (장비 -> 소모품 순 등)
    NewInventory.Items.Sort([](const FERNShopItemData& A, const FERNShopItemData& B)
    {
        return A.ItemCategory < B.ItemCategory;
    });

    UE_LOG(LogShopProvider, Warning, TEXT("====================================="));
    UE_LOG(LogShopProvider, Warning, TEXT("[%s] 고정 상점 생성 완료. 총 %d개 아이템."), *ShopID.ToString(), NewInventory.Items.Num());
    UE_LOG(LogShopProvider, Warning, TEXT("====================================="));

    return NewInventory;
}

void UERNDataTableShopProvider::RequestShopData_Implementation(EShopType ShopType)
{
    // [리팩토링 페이즈 2] 
    // 인터페이스가 ShopType만 받으므로 여기서는 처리 불가능합니다. (페이즈 4에서 Component가 우회하여 접근 예정)
    FERNShopInventory EmptyInventory;
    EmptyInventory.ShopType = ShopType;
    OnShopDataReceived.Broadcast(EmptyInventory);
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
    if (bIsDataCached && CachedShopData.Contains(Transaction.ShopID))
    {
        FERNShopInventory& Inventory = CachedShopData[Transaction.ShopID];
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
    // 페이즈 4 통합 전까지는 인터페이스 호환을 위해 빈 인벤토리를 반환합니다. 
    // 실제 캐시 접근은 ShopID 기반으로 직접(Casting) 이루어집니다.
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
