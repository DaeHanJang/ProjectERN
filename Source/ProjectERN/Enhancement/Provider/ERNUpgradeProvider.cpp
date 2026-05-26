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

    // ItemManagerSubsystem 캐시 (아이템 테이블 직접 조회용)
    if (UWorld* World = Owner ? Owner->GetWorld() : nullptr)
    {
        if (UGameInstance* GI = World->GetGameInstance())
        {
            CachedItemMgr = GI->GetSubsystem<UItemManagerSubsystem>();
        }
    }

    if (!CachedItemMgr)
    {
        UE_LOG(LogUpgrade, Error, TEXT("[UpgradeProvider] ItemManagerSubsystem 획득 실패!"));
        return;
    }

    UE_LOG(LogUpgrade, Log, TEXT("[UpgradeProvider] 초기화 완료. 아이템 테이블 NextGradeItemID 기반 조회 방식 사용."));
}

bool UERNUpgradeProvider::GetUpgradeInfo_Implementation(FName SourceItemID, FName& OutResultItemID, FName& OutMaterialID)
{
    if (!CachedItemMgr)
    {
        UE_LOG(LogUpgrade, Warning, TEXT("[UpgradeProvider] ItemManagerSubsystem이 없습니다."));
        return false;
    }

    const FERNItemTable* SourceRow = CachedItemMgr->FindItemRow(SourceItemID);
    if (!SourceRow || SourceRow->NextGradeItemID.IsNone())
    {
        return false;  // 강화 경로 없음 (최종 등급 또는 월드 획득)
    }

    OutResultItemID = SourceRow->NextGradeItemID;
    OutMaterialID   = SourceRow->UpgradeMaterialID;

    UE_LOG(LogUpgrade, Log, TEXT("[UpgradeProvider] 강화 경로 조회: %s → %s (재료: %s x1)"),
        *SourceItemID.ToString(), *OutResultItemID.ToString(),
        *OutMaterialID.ToString());

    return true;
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

    // 1. 아이템 테이블에서 강화 경로 직접 조회
    FName ResultItemID, MaterialID;

    if (!GetUpgradeInfo_Implementation(Transaction.SourceItemID, ResultItemID, MaterialID))
    {
        Transaction.Result = EUpgradeResult::NoUpgradePath;
        OnUpgradeComplete.Broadcast(Transaction);
        return;
    }

    Transaction.ResultItemID = ResultItemID;
    Transaction.MaterialItemID = MaterialID;

    // 2. 인벤토리 컴포넌트 획득
    UERNInventoryComponent* InvComp = ERNChar->GetInventoryComponent();
    if (!InvComp)
    {
        Transaction.Result = EUpgradeResult::InventoryError;
        OnUpgradeComplete.Broadcast(Transaction);
        return;
    }

    // 3. 재료(단석) 보유 확인 (1개 이상 필요)
    const TArray<FInventoryItemEntry>& Items = InvComp->GetInventory().GetItems();
    int32 MaterialSlotIndex = INDEX_NONE;

    for (int32 i = 0; i < Items.Num(); ++i)
    {
        if (Items[i].GetItemID() == MaterialID && Items[i].GetQuantity() >= 1)
        {
            MaterialSlotIndex = i;
            break;
        }
    }

    if (MaterialSlotIndex == INDEX_NONE)
    {
        Transaction.Result = EUpgradeResult::MaterialInsufficient;
        OnUpgradeComplete.Broadcast(Transaction);
        return;
    }

    // 4. 재료 1개 차감
    FItemRuntimeState DropState;
    FInventoryItemEntry ChangedMaterialEntry;
    InvComp->GetInventory().RemoveItem(MaterialSlotIndex, 1, DropState, ChangedMaterialEntry);
    InvComp->OnInventorySlotChanged.Broadcast(ChangedMaterialEntry);

    // 5. 무기 교체 (ChangeItem 활용)
    FItemRuntimeState NewWeaponState;
    NewWeaponState.SetItemID(ResultItemID);
    NewWeaponState.SetQuantity(1);

    FItemRuntimeState OldState = InvComp->GetInventory().ChangeItem(Transaction.SlotIndex, NewWeaponState);

    if (!OldState.IsValid())
    {
        // 롤백: 재료 1개 복구
        FItemRuntimeState RestoreMaterial;
        RestoreMaterial.SetItemID(MaterialID);
        RestoreMaterial.SetQuantity(1);

        TArray<FInventoryItemEntry> RestoredEntries;
        int32 MaxStack = CachedItemMgr ? CachedItemMgr->FindItemRow(MaterialID)->MaxStackSize : 99;
        InvComp->GetInventory().AddItem(RestoreMaterial, InvComp->GetMaxSlotSize(), MaxStack, RestoredEntries);

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

    UE_LOG(LogUpgrade, Log, TEXT("[UpgradeProvider] 강화 성공: %s → %s (재료 %s x1 소모)"),
        *Transaction.SourceItemID.ToString(), *Transaction.ResultItemID.ToString(),
        *MaterialID.ToString());
}

bool UERNUpgradeProvider::IsDataReady_Implementation()
{
    return CachedItemMgr != nullptr;
}
