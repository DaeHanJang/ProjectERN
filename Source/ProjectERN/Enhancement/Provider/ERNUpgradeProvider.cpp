#include "Enhancement/Provider/ERNUpgradeProvider.h"
#include "Inventory/Components/ERNInventoryComponent.h"
#include "Inventory/Item/Manager/ItemManagerSubsystem.h"
#include "Inventory/Item/Data/ERNItemTable.h"
#include "Inventory/Item/Data/EquipableItemDataAsset.h"
#include "Inventory/Data/ERNInventoryList.h"
#include "Character/Player/ProjectERNCharacter.h"

UERNUpgradeProvider::UERNUpgradeProvider()
{
}

void UERNUpgradeProvider::Initialize_Implementation(UObject* Owner)
{
    OwnerObject = Owner;

    if (!UpgradePathTable)
    {
        UE_LOG(LogUpgrade, Error, TEXT("[UpgradeProvider] UpgradePathTable이 할당되지 않았습니다!"));
        return;
    }

    // 테이블의 모든 Row를 순회하여 SourceItemID 기준으로 캐싱
    TArray<FERNUpgradePathTable*> AllPaths;
    UpgradePathTable->GetAllRows<FERNUpgradePathTable>(TEXT("UpgradeProviderCache"), AllPaths);

    for (FERNUpgradePathTable* PathData : AllPaths)
    {
        if (!PathData || PathData->SourceItemID.IsNone()) continue;

        CachedPaths.Add(PathData->SourceItemID, *PathData);

        UE_LOG(LogUpgrade, Log, TEXT("[UpgradeProvider] 캐싱: %s → %s (재료: %s x%d)"),
            *PathData->SourceItemID.ToString(),
            *PathData->ResultItemID.ToString(),
            *PathData->RequiredMaterialID.ToString(),
            PathData->RequiredMaterialCount);
    }

    bIsDataCached = true;
    UE_LOG(LogUpgrade, Log, TEXT("[UpgradeProvider] 초기화 완료. %d개의 강화 경로 캐싱됨."), CachedPaths.Num());
}

bool UERNUpgradeProvider::GetUpgradePath_Implementation(FName SourceItemID, FERNUpgradePathTable& OutPath)
{
    if (FERNUpgradePathTable* Found = CachedPaths.Find(SourceItemID))
    {
        OutPath = *Found;
        return true;
    }
    return false;
}

void UERNUpgradeProvider::ProcessUpgrade_Implementation(FERNUpgradeTransaction Transaction, ACharacter* PlayerChar)
{
    AProjectERNCharacter* ERNChar = Cast<AProjectERNCharacter>(PlayerChar);
    if (!ERNChar)
    {
        Transaction.Result = EUpgradeResult::InvalidItem;
        OnUpgradeComplete.Broadcast(Transaction);
        return;
    }

    // 1. 강화 경로 검증
    FERNUpgradePathTable Path;
    if (!GetUpgradePath_Implementation(Transaction.SourceItemID, Path))
    {
        Transaction.Result = EUpgradeResult::NoUpgradePath;
        OnUpgradeComplete.Broadcast(Transaction);
        return;
    }

    Transaction.ResultItemID = Path.ResultItemID;
    Transaction.MaterialItemID = Path.RequiredMaterialID;
    Transaction.MaterialCost = Path.RequiredMaterialCount;

    // 2. 인벤토리 컴포넌트 획득
    UERNInventoryComponent* InvComp = ERNChar->GetInventoryComponent();
    if (!InvComp)
    {
        Transaction.Result = EUpgradeResult::InventoryError;
        OnUpgradeComplete.Broadcast(Transaction);
        return;
    }

    // 3. 재료(단석) 보유량 확인
    // 인벤토리에서 RequiredMaterialID를 가진 슬롯 검색 및 수량 합산
    const TArray<FInventoryItemEntry>& Items = InvComp->GetInventory().GetItems();
    int32 MaterialSlotIndex = INDEX_NONE;
    int32 TotalMaterialCount = 0;

    for (int32 i = 0; i < Items.Num(); ++i)
    {
        if (Items[i].GetItemID() == Path.RequiredMaterialID)
        {
            TotalMaterialCount += Items[i].GetQuantity();
            if (MaterialSlotIndex == INDEX_NONE)
            {
                MaterialSlotIndex = i;
            }
        }
    }

    if (TotalMaterialCount < Path.RequiredMaterialCount)
    {
        Transaction.Result = EUpgradeResult::MaterialInsufficient;
        OnUpgradeComplete.Broadcast(Transaction);
        return;
    }

    // 4. 재료 차감
    FItemRuntimeState DropState;
    FInventoryItemEntry ChangedMaterialEntry;
    InvComp->GetInventory().RemoveItem(MaterialSlotIndex, Path.RequiredMaterialCount, DropState, ChangedMaterialEntry);
    InvComp->OnInventorySlotChanged.Broadcast(ChangedMaterialEntry);

    // 5. 무기 교체 (ChangeItem 활용)
    FItemRuntimeState NewWeaponState;
    NewWeaponState.SetItemID(Path.ResultItemID);
    NewWeaponState.SetQuantity(1);

    FItemRuntimeState OldState = InvComp->GetInventory().ChangeItem(Transaction.SlotIndex, NewWeaponState);

    if (!OldState.IsValid())
    {
        // 롤백: 재료 복구
        FItemRuntimeState RestoreMaterial;
        RestoreMaterial.SetItemID(Path.RequiredMaterialID);
        RestoreMaterial.SetQuantity(Path.RequiredMaterialCount);

        TArray<FInventoryItemEntry> RestoredEntries;
        UItemManagerSubsystem* ItemMgr = GetWorld()->GetGameInstance()->GetSubsystem<UItemManagerSubsystem>();
        int32 MaxStack = ItemMgr ? ItemMgr->FindItemRow(Path.RequiredMaterialID)->MaxStackSize : 99;
        InvComp->GetInventory().AddItem(RestoreMaterial, InvComp->GetMaxStackSize(), MaxStack, RestoredEntries);

        Transaction.Result = EUpgradeResult::InventoryError;
        OnUpgradeComplete.Broadcast(Transaction);
        return;
    }

    // 6. 교체된 슬롯 UI 갱신
    const TArray<FInventoryItemEntry>& UpdatedItems = InvComp->GetInventory().GetItems();
    if (UpdatedItems.IsValidIndex(Transaction.SlotIndex))
    {
        InvComp->OnInventorySlotChanged.Broadcast(UpdatedItems[Transaction.SlotIndex]);
    }

    // 7. 성공
    Transaction.Result = EUpgradeResult::Success;
    OnUpgradeComplete.Broadcast(Transaction);

    UE_LOG(LogUpgrade, Log, TEXT("[UpgradeProvider] 강화 성공: %s → %s"),
        *Transaction.SourceItemID.ToString(), *Transaction.ResultItemID.ToString());
}

bool UERNUpgradeProvider::IsDataReady_Implementation()
{
    return bIsDataCached;
}
