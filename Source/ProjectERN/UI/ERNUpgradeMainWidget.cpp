#include "UI/ERNUpgradeMainWidget.h"
#include "UI/ERNInventorySlotWidget.h"
#include "UI/ERNUpgradePreviewWidget.h"
#include "UI/ERNUpgradeMaterialTooltipWidget.h"
#include "Enhancement/Components/ERNUpgradeComponent.h"
#include "Inventory/Components/ERNInventoryComponent.h"
#include "Inventory/Item/Manager/ItemManagerSubsystem.h"
#include "Inventory/Item/Data/ERNItemTable.h"
#include "Character/Player/ProjectERNCharacter.h"
#include "Components/UniformGridPanel.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Components/UniformGridSlot.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "Inventory/Item/Data/ItemDataAssetBase.h"
#include "GameFramework/Character.h"
#include "Inventory/Components/ERNEquipmentComponent.h"

void UERNUpgradeMainWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // 캐릭터에서 컴포넌트 참조 획득
    if (ACharacter* PlayerChar = Cast<ACharacter>(GetOwningPlayerPawn()))
    {
        InventoryRef = PlayerChar->FindComponentByClass<UERNInventoryComponent>();
        UpgradeCompRef = PlayerChar->FindComponentByClass<UERNUpgradeComponent>();
        
        if (UERNEquipmentComponent* EquipComp = PlayerChar->FindComponentByClass<UERNEquipmentComponent>())
        {
            EquipComp->OnEquipmentSlotChanged.AddUniqueDynamic(this, &UERNUpgradeMainWidget::OnEquipableSlotChanged);
            OnEquipableSlotChanged(EquipComp->EquipableSlot);
        }
    }

    // 강화 결과 콜백 바인딩
    if (UpgradeCompRef)
    {
        UpgradeCompRef->OnUpgradeResult.AddDynamic(this, &UERNUpgradeMainWidget::OnUpgradeResultReceived);
    }
    
    // 인벤토리 동기화 완료 이벤트 바인딩
    if (InventoryRef)
    {
        InventoryRef->OnInventorySlotChanged.AddDynamic(this, &UERNUpgradeMainWidget::OnInventorySlotChanged);
    }

    // 장착 무기 슬롯 초기화
    if (EquipableSlotWidget)
    {
        EquipableSlotWidget->SetSlotIndex(-2);
        EquipableSlotWidget->OnSlotHovered.AddDynamic(this, &UERNUpgradeMainWidget::OnSlotHoveredCallback);
        EquipableSlotWidget->OnSlotUnhovered.AddDynamic(this, &UERNUpgradeMainWidget::OnSlotUnhoveredCallback);
        EquipableSlotWidget->OnSlotClicked.AddDynamic(this, &UERNUpgradeMainWidget::OnSlotClickedCallback);
        EquipableSlotWidget->OnSlotDoubleClicked.AddDynamic(this, &UERNUpgradeMainWidget::OnSlotDoubleClickedCallback);
    }

    // 강화 실행 버튼 바인딩
    if (UpgradeButton)
    {
        UpgradeButton->OnClicked.AddDynamic(this, &UERNUpgradeMainWidget::OnUpgradeConfirmed);
    }

    // 포커스 관련 설정 (키보드 조작을 위함)
    SetIsFocusable(true);

    // 슬롯 렌더링
    PopulateInventorySlots();
}

void UERNUpgradeMainWidget::NativeDestruct()
{
    if (UpgradeCompRef)
    {
        UpgradeCompRef->OnUpgradeResult.RemoveDynamic(this, &UERNUpgradeMainWidget::OnUpgradeResultReceived);
    }
    
    if (InventoryRef)
    {
        InventoryRef->OnInventorySlotChanged.RemoveDynamic(this, &UERNUpgradeMainWidget::OnInventorySlotChanged);
    }
    
    Super::NativeDestruct();
}

void UERNUpgradeMainWidget::PopulateInventorySlots()
{
    ClearSelection();

    if (!UpgradeInventoryGrid || !InventoryRef || !SlotWidgetClass) return;

    UpgradeInventoryGrid->ClearChildren();
    SlotWidgets.Empty();

    UItemManagerSubsystem* ItemMgr = GetWorld()->GetGameInstance()->GetSubsystem<UItemManagerSubsystem>();
    if (!ItemMgr) return;

    const int32 MaxSlotSize = InventoryRef->GetMaxSlotSize();
    const TArray<FInventoryItemEntry>& Items = InventoryRef->GetInventory().GetItems();

    for (int32 i = 0; i < MaxSlotSize; ++i)
    {
        UERNInventorySlotWidget* NewSlotWidget = CreateWidget<UERNInventorySlotWidget>(GetOwningPlayer(), SlotWidgetClass);
        if (NewSlotWidget)
        {
            NewSlotWidget->SetSlotIndex(i);
            
            // 인벤토리와 동일하게 이벤트 바인딩
            NewSlotWidget->OnSlotHovered.AddDynamic(this, &UERNUpgradeMainWidget::OnSlotHoveredCallback);
            NewSlotWidget->OnSlotUnhovered.AddDynamic(this, &UERNUpgradeMainWidget::OnSlotUnhoveredCallback);
            NewSlotWidget->OnSlotClicked.AddDynamic(this, &UERNUpgradeMainWidget::OnSlotClickedCallback);
            NewSlotWidget->OnSlotDoubleClicked.AddDynamic(this, &UERNUpgradeMainWidget::OnSlotDoubleClickedCallback);
            
            int32 Row = i / ColumnSize;
            int32 Column = i % ColumnSize;
            if (UUniformGridSlot* GridSlot = UpgradeInventoryGrid->AddChildToUniformGrid(NewSlotWidget, Row, Column))
            {
                GridSlot->SetHorizontalAlignment(HAlign_Fill);
                GridSlot->SetVerticalAlignment(VAlign_Fill);
            }
            SlotWidgets.Add(NewSlotWidget);

            // 데이터 채우기
            if (Items.IsValidIndex(i) && Items[i].IsValid())
            {
                const FERNItemTable* RowData = ItemMgr->FindItemRow(Items[i].GetItemID());
                if (RowData && RowData->ItemType == EItemType::Equipable)
                {
                    const UItemDataAssetBase* DataAsset = ItemMgr->LoadItemDataAssetSync(Items[i].GetItemID(), EItemAssetLoadFlags::UI);
                    if (DataAsset)
                    {
                        UTexture2D* IconTex = DataAsset->Icon.IsValid() ? DataAsset->Icon.Get() : DataAsset->Icon.LoadSynchronous();
                        NewSlotWidget->SetItem(IconTex, Items[i].GetQuantity(), ItemGradeByColor(RowData->Grade));
                    }
                }
                else
                {
                    // 장비가 아닌 아이템 (소모품 등)
                    const UItemDataAssetBase* DataAsset = ItemMgr->LoadItemDataAssetSync(Items[i].GetItemID(), EItemAssetLoadFlags::UI);
                    if (DataAsset)
                    {
                        UTexture2D* IconTex = DataAsset->Icon.IsValid() ? DataAsset->Icon.Get() : DataAsset->Icon.LoadSynchronous();
                        NewSlotWidget->SetItem(IconTex, Items[i].GetQuantity(), ItemGradeByColor(RowData->Grade));
                    }
                        
                    // 장비가 아님을 나타내기 위해 어둡게 처리 (단, 클릭은 막지 않음. 로직에서 컷트)
                    NewSlotWidget->SetColorAndOpacity(FLinearColor(0.2f, 0.2f, 0.2f, 1.0f));
                }
            }
            else
            {
                NewSlotWidget->ClearItem();
            }
            
            NewSlotWidget->InitInventorySlotTint();
        }
    }
}

void UERNUpgradeMainWidget::UpdateVisuals()
{
    // 1. 모든 슬롯을 기본 상태로 초기화 (인벤토리 동일화)
    for (int32 i = 0; i < SlotWidgets.Num(); ++i)
    {
        if (UERNInventorySlotWidget* SlotWidget = SlotWidgets[i])
        {
            SlotWidget->SetInventorySlotImage(InventorySlotImage.Get());
            SlotWidget->InitInventorySlotTint();
        }
    }

    if (EquipableSlotWidget)
    {
        EquipableSlotWidget->SetInventorySlotImage(InventorySlotImage.Get());
        EquipableSlotWidget->InitInventorySlotTint();
    }

    // 2. 표시할 단일 인덱스 결정 (호버 우선)
    int32 DisplayIndex = -1;
    if (HoveredSlotIndex != -1)
    {
        DisplayIndex = HoveredSlotIndex;
    }
    else if (FocusSlotIndex != -1)
    {
        DisplayIndex = FocusSlotIndex;
    }

    // 3. 포커스 이미지 및 틴트 적용
    if (DisplayIndex != -1)
    {
        if (DisplayIndex == -2 && EquipableSlotWidget)
        {
            EquipableSlotWidget->SetInventorySlotImage(FocusSlotImage.Get());
            EquipableSlotWidget->SetInventorySlotTint(FColor::White);
        }
        else if (SlotWidgets.IsValidIndex(DisplayIndex))
        {
            SlotWidgets[DisplayIndex]->SetInventorySlotImage(FocusSlotImage.Get());
            SlotWidgets[DisplayIndex]->SetInventorySlotTint(FColor::White);
        }
    }

    // 2. 프리뷰 갱신 (호버된 슬롯 무시, 클릭으로 포커스 잡힌 슬롯만 표시)
    const int32 PreviewIndex = FocusSlotIndex;
    
    if (PreviewIndex == -1)
    {
        ClearSelection();
        return;
    }

    // 장비 검사
    if (InventoryRef && InventoryRef->GetInventory().GetItems().IsValidIndex(PreviewIndex))
    {
        const FInventoryItemEntry& Entry = InventoryRef->GetInventory().GetItems()[PreviewIndex];
        if (!Entry.IsValid()) 
        {
            ClearSelection();
            return;
        }

        UItemManagerSubsystem* ItemMgr = GetWorld()->GetGameInstance()->GetSubsystem<UItemManagerSubsystem>();
        if (ItemMgr)
        {
            const FERNItemTable* Row = ItemMgr->FindItemRow(Entry.GetItemID());
            if (!Row || Row->ItemType != EItemType::Equipable)
            {
                ClearSelection();
                return;
            }
        }
    }
    else
    {
        ClearSelection();
        return;
    }

    // 프리뷰 패널 표시
    if (PreviewContainer) PreviewContainer->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

    if (!PreviewContainer)
    {
        if (BeforePreviewWidget) BeforePreviewWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
        if (AfterPreviewWidget) AfterPreviewWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
        if (MaterialTooltipWidget) MaterialTooltipWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
        if (PreviewArrow) PreviewArrow->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
        if (BeforeTitleText) BeforeTitleText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
        if (AfterTitleText) AfterTitleText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
        if (UpgradeButton) UpgradeButton->SetVisibility(ESlateVisibility::Visible);
    }
    
    if (MaterialNameText) MaterialNameText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    if (MaterialCountText) MaterialCountText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

    if (!UpgradeCompRef) return;

    FERNUpgradePreview Preview = UpgradeCompRef->GetUpgradePreview(PreviewIndex);
    const bool bCannotUpgrade = Preview.ResultItemID.IsNone();

    // 강화 가능 여부에 따른 컴포넌트 숨김/표시
    if (bCannotUpgrade)
    {
        if (BeforeTitleText) BeforeTitleText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
        if (AfterPreviewWidget) AfterPreviewWidget->SetVisibility(ESlateVisibility::Hidden);
        if (AfterTitleText) AfterTitleText->SetVisibility(ESlateVisibility::Hidden);
        if (MaterialTooltipWidget) MaterialTooltipWidget->SetVisibility(ESlateVisibility::Hidden);
        if (PreviewArrow) PreviewArrow->SetVisibility(ESlateVisibility::Hidden);
        if (UpgradeButton) UpgradeButton->SetVisibility(ESlateVisibility::Hidden);
        if (MaterialNameText) MaterialNameText->SetVisibility(ESlateVisibility::Hidden);
        if (MaterialCountText) MaterialCountText->SetVisibility(ESlateVisibility::Hidden);
    }
    else
    {
        if (BeforeTitleText) BeforeTitleText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
        if (AfterTitleText) AfterTitleText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
        if (AfterPreviewWidget) AfterPreviewWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
        if (MaterialTooltipWidget) MaterialTooltipWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
        if (PreviewArrow) PreviewArrow->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
        if (UpgradeButton) UpgradeButton->SetVisibility(ESlateVisibility::Visible);
        if (MaterialNameText) MaterialNameText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
        if (MaterialCountText) MaterialCountText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    }

    // 아이콘 로딩 및 설정
    UTexture2D* SourceIconTexture = Preview.SourceIcon.IsValid() ? Preview.SourceIcon.Get() : Preview.SourceIcon.LoadSynchronous();
    UTexture2D* ResultIconTexture = Preview.ResultIcon.IsValid() ? Preview.ResultIcon.Get() : Preview.ResultIcon.LoadSynchronous();

    if (BeforePreviewWidget)
    {
        BeforePreviewWidget->SetPreviewData(Preview.SourceDisplayName, Preview.SourceGrade, Preview.SourceLightAttack, Preview.SourceHeavyAttack, false, SourceIconTexture);
    }

    if (!bCannotUpgrade)
    {
        if (AfterPreviewWidget)
        {
            AfterPreviewWidget->SetPreviewData(Preview.ResultDisplayName, Preview.ResultGrade, Preview.ResultLightAttack, Preview.ResultHeavyAttack, true, ResultIconTexture);
        }

        UTexture2D* MatIconTexture = Preview.MaterialIcon.IsValid() ? Preview.MaterialIcon.Get() : Preview.MaterialIcon.LoadSynchronous();

        if (MaterialTooltipWidget)
        {
            MaterialTooltipWidget->SetMaterialData(Preview.MaterialDisplayName, Preview.MaterialGrade, Preview.CurrentMaterialCount, MatIconTexture);
        }

        if (MaterialNameText) MaterialNameText->SetText(Preview.MaterialDisplayName);
        if (MaterialCountText) MaterialCountText->SetText(FText::Format(NSLOCTEXT("Upgrade", "MatCountFormat", "x1 ({0})"), FText::AsNumber(Preview.CurrentMaterialCount)));
    }
}

void UERNUpgradeMainWidget::OnSlotHoveredCallback(const int32 Index)
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(UnhoverTimerHandle);
    }
    
    HoveredSlotIndex = Index;
    UpdateVisuals();
}

void UERNUpgradeMainWidget::OnSlotUnhoveredCallback(const int32 Index)
{
    if (HoveredSlotIndex == Index)
    {
        HoveredSlotIndex = -1;
        
        // 깜빡임 방지 (인벤토리 로직)
        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().SetTimer(UnhoverTimerHandle, this, &UERNUpgradeMainWidget::UnhoverDelay, 0.05f, false);
        }
        else
        {
            UpdateVisuals();
        }
    }
}

void UERNUpgradeMainWidget::UnhoverDelay()
{
    if (HoveredSlotIndex == -1)
    {
        UpdateVisuals();
    }
}

void UERNUpgradeMainWidget::OnSlotClickedCallback(const int32 Index)
{
    FocusSlotIndex = Index;
    UpdateVisuals();
}

void UERNUpgradeMainWidget::OnSlotDoubleClickedCallback(const int32 Index)
{
    FocusSlotIndex = Index;
    UpdateVisuals();
    OnUpgradeConfirmed();
}

void UERNUpgradeMainWidget::ClearSelection()
{
    FocusSlotIndex = -1;
    HoveredSlotIndex = -1;

    // 패널 전체 숨김 (화살표 포함)
    if (PreviewContainer) PreviewContainer->SetVisibility(ESlateVisibility::Hidden);

    if (!PreviewContainer)
    {
        if (BeforePreviewWidget) BeforePreviewWidget->SetVisibility(ESlateVisibility::Hidden);
        if (AfterPreviewWidget) AfterPreviewWidget->SetVisibility(ESlateVisibility::Hidden);
        if (MaterialTooltipWidget) MaterialTooltipWidget->SetVisibility(ESlateVisibility::Hidden);
        if (PreviewArrow) PreviewArrow->SetVisibility(ESlateVisibility::Hidden);
        if (BeforeTitleText) BeforeTitleText->SetVisibility(ESlateVisibility::Hidden);
        if (AfterTitleText) AfterTitleText->SetVisibility(ESlateVisibility::Hidden);
        if (UpgradeButton) UpgradeButton->SetVisibility(ESlateVisibility::Hidden);
    }
    
    if (MaterialNameText) MaterialNameText->SetVisibility(ESlateVisibility::Hidden);
    if (MaterialCountText) MaterialCountText->SetVisibility(ESlateVisibility::Hidden);
    
    // 시각적 상태 초기화 
    for (UERNInventorySlotWidget* SlotWidget : SlotWidgets)
    {
        if (SlotWidget)
        {
            SlotWidget->SetInventorySlotImage(InventorySlotImage.Get());
            SlotWidget->InitInventorySlotTint();
        }
    }
    
    if (EquipableSlotWidget)
    {
        EquipableSlotWidget->SetInventorySlotImage(InventorySlotImage.Get());
        EquipableSlotWidget->InitInventorySlotTint();
    }
}

void UERNUpgradeMainWidget::OnUpgradeConfirmed()
{
    // 강화는 포커스 잡힌(우측에 표시중인) 슬롯 기준
    if (FocusSlotIndex == -1 || !UpgradeCompRef) return;
    UpgradeCompRef->TryUpgradeItem(FocusSlotIndex);
}

void UERNUpgradeMainWidget::OnUpgradeResultReceived(const FERNUpgradeTransaction& Transaction)
{
    if (Transaction.Result == EUpgradeResult::Success)
    {
        BP_OnUpgradeSuccess();
    }
}

void UERNUpgradeMainWidget::OnInventorySlotChanged(const FInventoryItemEntry& Entry)
{
    // 장비 상태가 변했으므로 인벤토리 슬롯 전체 재구성 및 현재 프리뷰 재조회
    PopulateInventorySlots();
    UpdateVisuals();
}

void UERNUpgradeMainWidget::OnEquipableSlotChanged(const FInventoryItemEntry& Entry)
{
    if (!EquipableSlotWidget) return;
    
    if (Entry.GetItemID().IsNone() || Entry.GetItemID() == NAME_None)
    {
        EquipableSlotWidget->ClearItem();
    }
    else
    {
        UItemManagerSubsystem* ItemMgr = GetWorld()->GetGameInstance()->GetSubsystem<UItemManagerSubsystem>();
        if (ItemMgr)
        {
            const UItemDataAssetBase* DataAsset = ItemMgr->LoadItemDataAssetSync(Entry.GetItemID(), EItemAssetLoadFlags::UI);
            if (DataAsset)
            {
                UTexture2D* IconTex = DataAsset->Icon.IsValid() ? DataAsset->Icon.Get() : DataAsset->Icon.LoadSynchronous();
                const FERNItemTable* RowData = ItemMgr->FindItemRow(Entry.GetItemID());
                EItemGrade Grade = RowData ? RowData->Grade : EItemGrade::Common;
                EquipableSlotWidget->SetItem(IconTex, Entry.GetQuantity(), ItemGradeByColor(Grade));
            }
        }
    }
    
    if (FocusSlotIndex == -2)
    {
        UpdateVisuals();
    }
}

FReply UERNUpgradeMainWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
    // 상호작용은 포커스 잡힌 슬롯 기준
    if (FocusSlotIndex != -1)
    {
        if (InKeyEvent.GetKey() == EKeys::Escape)
        {
            ClearSelection();
            return FReply::Handled();
        }
        
        if (InKeyEvent.GetKey() == EKeys::E)
        {
            OnUpgradeConfirmed();
            // 포커스는 그대로 둬서 강화 후 갱신된 프리뷰를 계속 볼 수 있게 함
            return FReply::Handled();
        }
    }

    return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

FReply UERNUpgradeMainWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton || InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
    {
        ClearSelection();
        return FReply::Handled();
    }
    
    return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UERNUpgradeMainWidget::NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
    const int32 ActiveIndex = (HoveredSlotIndex != -1) ? HoveredSlotIndex : FocusSlotIndex;
    if (ActiveIndex != -1 && InventoryRef)
    {
        const int32 NextIndex = GetNavigationTargetSlotIndex(InKeyEvent.GetKey(), InventoryRef->GetMaxSlotSize(), ActiveIndex);
        if (NextIndex != INDEX_NONE)
        {           
            FocusSlotIndex = NextIndex;
            UpdateVisuals();
            return FReply::Handled();
        }
    }
    
    return Super::NativeOnPreviewKeyDown(InGeometry, InKeyEvent);
}

const int32 UERNUpgradeMainWidget::GetNavigationTargetSlotIndex(const FKey& Key, const int32 MaxSlotSize, const int32 CurrentIndex) const
{
    int32 NextIndex = INDEX_NONE;
    
    if (Key == EKeys::Up)
    {
        NextIndex = (CurrentIndex - ColumnSize < 0) ? CurrentIndex + ColumnSize * (((CurrentIndex - ColumnSize) * -1 + MaxSlotSize - 1) / ColumnSize - 1) : CurrentIndex - ColumnSize;
    }
    else if (Key == EKeys::Down)
    {
        NextIndex = CurrentIndex + ColumnSize >= MaxSlotSize ? (CurrentIndex + ColumnSize) % ColumnSize : CurrentIndex + ColumnSize;
    }
    else if (Key == EKeys::Left)
    {
        NextIndex = (CurrentIndex - 1 < 0) ? MaxSlotSize - 1 : CurrentIndex - 1;
    }
    else if (Key == EKeys::Right)
    {
        NextIndex = (CurrentIndex + 1) % MaxSlotSize;
    }
    
    return NextIndex;
}

FColor UERNUpgradeMainWidget::ItemGradeByColor(EItemGrade Grade)
{
    FColor Color;
    switch (Grade)
    {
    case EItemGrade::Uncommon:
        Color = FColor::Cyan;
        break;
    case EItemGrade::Rare:
        Color =  FColor::Purple;
        break;
    case EItemGrade::Legendary:
        Color =  FColor::Orange;
        break;
    default:
        Color =  FColor::White;
        break;
    }
    
    return Color;
}
