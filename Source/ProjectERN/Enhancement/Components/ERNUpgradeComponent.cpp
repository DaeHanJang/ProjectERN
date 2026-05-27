#include "Enhancement/Components/ERNUpgradeComponent.h"
#include "Enhancement/Provider/ERNUpgradeDataProvider.h"
#include "Enhancement/Provider/ERNUpgradeProvider.h"
#include "Inventory/Components/ERNInventoryComponent.h"
#include "Inventory/Item/Manager/ItemManagerSubsystem.h"
#include "Inventory/Item/Data/ERNItemTable.h"
#include "Inventory/Item/Data/EquipableItemDataAsset.h"
#include "GameFramework/Character.h"
#include "Net/UnrealNetwork.h"

// Phase 4 구현 완료: UERNGameInstance 포함 경로 추가
#include "Core/ERNGameInstance.h"
#include "Inventory/Components/ERNEquipmentComponent.h"

UERNUpgradeComponent::UERNUpgradeComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

void UERNUpgradeComponent::BeginPlay()
{
    Super::BeginPlay();
    AcquireProvider();
}

void UERNUpgradeComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

void UERNUpgradeComponent::AcquireProvider()
{
    if (UERNGameInstance* GI = Cast<UERNGameInstance>(GetWorld()->GetGameInstance()))
    {
        DataProvider = Cast<IERNUpgradeDataProvider>(GI->GetUpgradeDataProviderObject());

        if (DataProvider)
        {
            if (UERNUpgradeProvider* UpgProvider = Cast<UERNUpgradeProvider>(
                    Cast<UObject>(DataProvider)))
            {
                UpgProvider->OnUpgradeComplete.AddDynamic(this, &UERNUpgradeComponent::OnUpgradeComplete);
            }
            UE_LOG(LogUpgrade, Log, TEXT("[UpgradeComponent] Provider 획득 성공"));
        }
    }
}

// ===== 공개 API =====

void UERNUpgradeComponent::OpenUpgradeUI(AActor* TargetAnvil)
{
    CurrentTargetAnvil = TargetAnvil;
    bIsUpgradeOpen = true;
    UE_LOG(LogUpgrade, Log, TEXT("[UpgradeComponent] 강화 UI 열기"));
}

void UERNUpgradeComponent::CloseUpgradeUI()
{
    bIsUpgradeOpen = false;
    CurrentTargetAnvil = nullptr;
    UE_LOG(LogUpgrade, Log, TEXT("[UpgradeComponent] 강화 UI 닫기"));
}

FERNUpgradePreview UERNUpgradeComponent::GetUpgradePreview(int32 SlotIndex)
{
    FERNUpgradePreview Preview;

    ACharacter* OwnerChar = Cast<ACharacter>(GetOwner());
    if (!OwnerChar) return Preview;

    UERNInventoryComponent* InvComp = OwnerChar->FindComponentByClass<UERNInventoryComponent>();
    if (!InvComp) return Preview;

    UItemManagerSubsystem* ItemMgr = GetWorld()->GetGameInstance()->GetSubsystem<UItemManagerSubsystem>();
    if (!ItemMgr) return Preview;

    // 선택한 슬롯의 아이템 ID 획득
    FName SourceID;
    if (SlotIndex == -2)
    {
        UERNEquipmentComponent* EquipComp = OwnerChar->FindComponentByClass<UERNEquipmentComponent>();
        if (!EquipComp || !EquipComp->EquipableSlot.IsValid()) return Preview;
        SourceID = EquipComp->EquipableSlot.GetItemID();
    }
    else
    {
        const TArray<FInventoryItemEntry>& Items = InvComp->GetInventory().GetItems();
        if (!Items.IsValidIndex(SlotIndex) || !Items[SlotIndex].IsValid()) return Preview;
        SourceID = Items[SlotIndex].GetItemID();
    }

    Preview.SourceItemID = SourceID;

    // 소스 아이템 데이터 조회
    const FERNItemTable* SourceRow = ItemMgr->FindItemRow(SourceID);
    if (SourceRow)
    {
        Preview.SourceDisplayName = SourceRow->DisplayName;
        Preview.SourceGrade = SourceRow->Grade;

        // DataAsset에서 스탯 + 아이콘 읽기
        const UItemDataAssetBase* BaseAsset = ItemMgr->LoadItemDataAssetSync(SourceID, EItemAssetLoadFlags::UI);
        if (const UEquipableItemDataAsset* EquipAsset = Cast<UEquipableItemDataAsset>(BaseAsset))
        {
            Preview.SourceLightAttack = EquipAsset->LightAttackDamage;
        }
        if (BaseAsset)
        {
            Preview.SourceIcon = BaseAsset->Icon;
        }
    }

    // 강화 경로 조회: 아이템 테이블의 NextGradeItemID 직접 참조
    if (SourceRow && !SourceRow->NextGradeItemID.IsNone())
    {
        Preview.ResultItemID = SourceRow->NextGradeItemID;
        Preview.RequiredMaterialID = SourceRow->UpgradeMaterialID;

        // 결과 아이템 데이터 조회
        const FERNItemTable* ResultRow = ItemMgr->FindItemRow(SourceRow->NextGradeItemID);
        if (ResultRow)
        {
            Preview.ResultDisplayName = ResultRow->DisplayName;
            Preview.ResultGrade = ResultRow->Grade;

            const UItemDataAssetBase* ResultBase = ItemMgr->LoadItemDataAssetSync(SourceRow->NextGradeItemID, EItemAssetLoadFlags::UI);
            if (const UEquipableItemDataAsset* ResultEquip = Cast<UEquipableItemDataAsset>(ResultBase))
            {
                Preview.ResultLightAttack = ResultEquip->LightAttackDamage;
            }
            if (ResultBase)
            {
                Preview.ResultIcon = ResultBase->Icon;
            }
        }

        // 재료 데이터 조회 (이름 + 아이콘 + 등급)
        const FERNItemTable* MatRow = ItemMgr->FindItemRow(SourceRow->UpgradeMaterialID);
        if (MatRow)
        {
            Preview.MaterialDisplayName = MatRow->DisplayName;
            Preview.MaterialGrade = MatRow->Grade;

            const UItemDataAssetBase* MatAsset = ItemMgr->LoadItemDataAssetSync(SourceRow->UpgradeMaterialID, EItemAssetLoadFlags::UI);
            if (MatAsset)
            {
                Preview.MaterialIcon = MatAsset->Icon;
            }
        }

        // 현재 재료 보유량 합산
        Preview.CurrentMaterialCount = 0;
        const TArray<FInventoryItemEntry>& CurrentItems = InvComp->GetInventory().GetItems();
        for (const FInventoryItemEntry& Entry : CurrentItems)
        {
            if (Entry.GetItemID() == SourceRow->UpgradeMaterialID)
            {
                Preview.CurrentMaterialCount += Entry.GetQuantity();
            }
        }
        
        // 1개 이상 보유 시 강화 가능
        Preview.bCanUpgrade = (Preview.CurrentMaterialCount >= 1);
    }

    return Preview;
}

void UERNUpgradeComponent::TryUpgradeItem(int32 SlotIndex)
{
    UE_LOG(LogUpgrade, Log, TEXT("[UpgradeComponent] 강화 시도: 슬롯 %d"), SlotIndex);
    Server_RequestUpgrade(SlotIndex, CurrentTargetAnvil);
}

// ===== Server RPC =====

void UERNUpgradeComponent::Server_RequestUpgrade_Implementation(int32 SlotIndex, AActor* TargetAnvil)
{
    UE_LOG(LogUpgrade, Log, TEXT("[UpgradeComponent:Server] 강화 요청 수신: 슬롯 %d"), SlotIndex);

    ACharacter* PlayerChar = Cast<ACharacter>(GetOwner());
    if (!PlayerChar || !TargetAnvil)
    {
        UE_LOG(LogUpgrade, Warning, TEXT("[UpgradeComponent:Server] 유효하지 않은 요청"));
        return;
    }

    // 거리 검증 (핵 방지)
    float Distance = FVector::Dist(PlayerChar->GetActorLocation(), TargetAnvil->GetActorLocation());
    if (Distance > 500.f)
    {
        UE_LOG(LogUpgrade, Warning, TEXT("[UpgradeComponent:Server] 모루와 너무 멉니다 (거리: %f)"), Distance);
        return;
    }

    if (DataProvider)
    {
        FName SourceID;
        if (SlotIndex == -2)
        {
            UERNEquipmentComponent* EquipComp = PlayerChar->FindComponentByClass<UERNEquipmentComponent>();
            if (!EquipComp || !EquipComp->EquipableSlot.IsValid()) return;
            SourceID = EquipComp->EquipableSlot.GetItemID();
        }
        else
        {
            // 슬롯에서 아이템 ID 획득
            UERNInventoryComponent* InvComp = PlayerChar->FindComponentByClass<UERNInventoryComponent>();
            if (!InvComp) return;

            const TArray<FInventoryItemEntry>& Items = InvComp->GetInventory().GetItems();
            if (!Items.IsValidIndex(SlotIndex) || !Items[SlotIndex].IsValid()) return;
            SourceID = Items[SlotIndex].GetItemID();
        }

        FERNUpgradeTransaction Tx;
        Tx.SlotIndex = SlotIndex;
        Tx.SourceItemID = SourceID;
        Tx.RequestingPlayer = PlayerChar;
        Tx.Timestamp = GetWorld()->GetTimeSeconds();

        // Provider에게 검증/처리 위임
        IERNUpgradeDataProvider::Execute_ProcessUpgrade(
            Cast<UObject>(DataProvider), Tx, PlayerChar);
    }
}

// ===== Client RPC =====

void UERNUpgradeComponent::Client_UpgradeResult_Implementation(const FERNUpgradeTransaction& Transaction)
{
    UE_LOG(LogUpgrade, Log, TEXT("[UpgradeComponent:Client] 강화 결과 수신: %s (결과: %d)"),
        *Transaction.SourceItemID.ToString(), static_cast<int32>(Transaction.Result));

    OnUpgradeResult.Broadcast(Transaction);
    OnUpgradeUIUpdate.Broadcast();
}

// ===== 내부 콜백 =====

void UERNUpgradeComponent::OnUpgradeComplete(const FERNUpgradeTransaction& Transaction)
{
    if (GetOwner()->HasAuthority())
    {
        // 내 트랜잭션인지 확인
        if (Transaction.RequestingPlayer != GetOwner()) return;

        // 결과를 클라이언트에게 전송
        Client_UpgradeResult(Transaction);
    }
}
