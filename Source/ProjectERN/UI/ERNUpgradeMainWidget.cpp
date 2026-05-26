#include "UI/ERNUpgradeMainWidget.h"
#include "UI/ERNUpgradeSlotWidget.h"
#include "UI/ERNUpgradePreviewWidget.h"
#include "UI/ERNUpgradeMaterialTooltipWidget.h"
#include "Enhancement/Components/ERNUpgradeComponent.h"
#include "Inventory/Components/ERNInventoryComponent.h"
#include "Inventory/Item/Manager/ItemManagerSubsystem.h"
#include "Inventory/Item/Data/ERNItemTable.h"
#include "Character/Player/ProjectERNCharacter.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"

void UERNUpgradeMainWidget::NativeConstruct()
{
    Super::NativeConstruct();
    UE_LOG(LogUpgrade, Warning, TEXT("[DEBUG] UERNUpgradeMainWidget::NativeConstruct 시작"));

    // 캐릭터에서 컴포넌트 참조 획득
    if (AProjectERNCharacter* Char = Cast<AProjectERNCharacter>(GetOwningPlayerPawn()))
    {
        UpgradeCompRef = Char->GetUpgradeComponent();
        InventoryRef = Char->GetInventoryComponent();
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

    // 강화 실행 버튼 바인딩
    if (UpgradeButton)
    {
        UpgradeButton->OnClicked.AddDynamic(this, &UERNUpgradeMainWidget::OnUpgradeConfirmed);
    }

    // 인벤토리에서 장비 슬롯만 추출하여 표시
    UE_LOG(LogUpgrade, Warning, TEXT("[DEBUG] PopulateInventorySlots 호출 직전"));
    PopulateInventorySlots();
    UE_LOG(LogUpgrade, Warning, TEXT("[DEBUG] UERNUpgradeMainWidget::NativeConstruct 완료"));
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
    UE_LOG(LogUpgrade, Warning, TEXT("[DEBUG] PopulateInventorySlots 시작"));
    ClearSelection();

    if (!InventoryScrollBox || !InventoryRef || !SlotWidgetClass)
    {
        UE_LOG(LogUpgrade, Error, TEXT("[UpgradeMainWidget] 초기화 실패! ScrollBox:%d, InvRef:%d, SlotClass:%d"),
            InventoryScrollBox != nullptr, InventoryRef != nullptr, SlotWidgetClass != nullptr);
        return;
    }

    // 기존 슬롯들만 선택적으로 제거 (배경 이미지 등은 유지)
    for (int32 i = InventoryScrollBox->GetChildrenCount() - 1; i >= 0; --i)
    {
        if (Cast<UERNUpgradeSlotWidget>(InventoryScrollBox->GetChildAt(i)))
        {
            InventoryScrollBox->RemoveChildAt(i);
        }
    }
    UE_LOG(LogUpgrade, Warning, TEXT("[DEBUG] InventoryScrollBox 초기화 완료"));

    UItemManagerSubsystem* ItemMgr = GetWorld()->GetGameInstance()->GetSubsystem<UItemManagerSubsystem>();
    if (!ItemMgr)
    {
        UE_LOG(LogUpgrade, Error, TEXT("[DEBUG] ItemManagerSubsystem 획득 실패!"));
        return;
    }

    UE_LOG(LogUpgrade, Warning, TEXT("[DEBUG] 아이템 순회 시작. 개수: %d"), InventoryRef->GetInventory().GetItems().Num());
    const TArray<FInventoryItemEntry>& Items = InventoryRef->GetInventory().GetItems();
    for (int32 i = 0; i < Items.Num(); ++i)
    {
        if (!Items[i].IsValid()) continue;

        // 장비 아이템만 필터링
        const FERNItemTable* Row = ItemMgr->FindItemRow(Items[i].GetItemID());
        if (!Row || Row->ItemType != EItemType::Equipable) continue;

        // 슬롯 위젯 생성
        UERNUpgradeSlotWidget* NewSlotWidget = CreateWidget<UERNUpgradeSlotWidget>(GetOwningPlayer(), SlotWidgetClass);
        if (NewSlotWidget)
        {
            NewSlotWidget->SetupSlot(i, Items[i].GetItemID(), Row);
            NewSlotWidget->OnSlotClicked.AddDynamic(this, &UERNUpgradeMainWidget::OnSlotSelected);
            InventoryScrollBox->AddChild(NewSlotWidget);
        }
    }
}

void UERNUpgradeMainWidget::ClearSelection()
{
    SelectedSlotIndex = INDEX_NONE;

    // 패널 전체 숨김 (화살표 포함)
    if (PreviewContainer) PreviewContainer->SetVisibility(ESlateVisibility::Hidden);

    // 하위 위젯들을 개별 제어할 필요가 줄어들지만, 안전장치로 유지 (컨테이너가 없을 경우 대비)
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
    
    // 레거시 UI 숨김
    if (MaterialNameText) MaterialNameText->SetVisibility(ESlateVisibility::Hidden);
    if (MaterialCountText) MaterialCountText->SetVisibility(ESlateVisibility::Hidden);
}

void UERNUpgradeMainWidget::OnSlotSelected(int32 SlotIndex)
{
    SelectedSlotIndex = SlotIndex;

    // 패널 전체 표시
    if (PreviewContainer) PreviewContainer->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

    // 하위 위젯 복구 (컨테이너가 없을 경우 대비)
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

    // 미리보기 데이터 생성
    FERNUpgradePreview Preview = UpgradeCompRef->GetUpgradePreview(SlotIndex);

    UE_LOG(LogUpgrade, Warning, TEXT("[DEBUG] OnSlotSelected - ItemID: %s, SourceGrade: %d"), 
        *Preview.SourceItemID.ToString(), static_cast<int32>(Preview.SourceGrade));

    // === 강화 가능 여부 판정: NextGradeItemID 데이터 기반 ===
    // NextGradeItemID가 None → 강화 불가 (최종 등급 또는 월드 획득 전용)
    const bool bCannotUpgrade = Preview.ResultItemID.IsNone();

    if (bCannotUpgrade)
    {
        // 강화 전 정보만 표시
        if (BeforeTitleText) BeforeTitleText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

        // 강화 후, 재료, 화살표, 강화 버튼 숨김
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
        // 강화 가능: 전체 표시
        if (BeforeTitleText) BeforeTitleText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
        if (AfterTitleText) AfterTitleText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
        if (AfterPreviewWidget) AfterPreviewWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
        if (MaterialTooltipWidget) MaterialTooltipWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
        if (PreviewArrow) PreviewArrow->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
        if (UpgradeButton) UpgradeButton->SetVisibility(ESlateVisibility::Visible);
        if (MaterialNameText) MaterialNameText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
        if (MaterialCountText) MaterialCountText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    }

    // 프리뷰 위젯 갱신 (아이콘 포함)
    UTexture2D* SourceIconTexture = Preview.SourceIcon.IsValid() ? Preview.SourceIcon.Get() : Preview.SourceIcon.LoadSynchronous();
    UTexture2D* ResultIconTexture = Preview.ResultIcon.IsValid() ? Preview.ResultIcon.Get() : Preview.ResultIcon.LoadSynchronous();

    if (BeforePreviewWidget)
    {
        BeforePreviewWidget->SetPreviewData(
            Preview.SourceDisplayName, Preview.SourceGrade,
            Preview.SourceLightAttack, Preview.SourceHeavyAttack, false,
            SourceIconTexture);
    }

    // 강화 불가 등급이면 강화 후 프리뷰와 재료 갱신 스킵
    if (!bCannotUpgrade)
    {
        if (AfterPreviewWidget)
        {
            AfterPreviewWidget->SetPreviewData(
                Preview.ResultDisplayName, Preview.ResultGrade,
                Preview.ResultLightAttack, Preview.ResultHeavyAttack, true,
                ResultIconTexture);
        }

        // 재료(단석) 툴팁 위젯 갱신
        UTexture2D* MatIconTexture = Preview.MaterialIcon.IsValid() ? Preview.MaterialIcon.Get() : Preview.MaterialIcon.LoadSynchronous();

        if (MaterialTooltipWidget)
        {
            MaterialTooltipWidget->SetMaterialData(
                Preview.MaterialDisplayName, Preview.MaterialGrade,
                Preview.CurrentMaterialCount, MatIconTexture);
        }

        // 레거시 재료 텍스트 갱신
        if (MaterialNameText)
            MaterialNameText->SetText(Preview.MaterialDisplayName);
        if (MaterialCountText)
            MaterialCountText->SetText(FText::Format(
                NSLOCTEXT("Upgrade", "MatCountFormat", "x1 ({0})"),
                FText::AsNumber(Preview.CurrentMaterialCount)));
    }
}

void UERNUpgradeMainWidget::OnUpgradeConfirmed()
{
    if (SelectedSlotIndex == INDEX_NONE || !UpgradeCompRef) return;
    UpgradeCompRef->TryUpgradeItem(SelectedSlotIndex);
}

void UERNUpgradeMainWidget::OnUpgradeResultReceived(const FERNUpgradeTransaction& Transaction)
{
    // 성공 시 이벤트 연출만 처리, 슬롯 갱신은 OnInventorySlotChanged가 담당하여 최신 데이터 적용
    if (Transaction.Result == EUpgradeResult::Success)
    {
        BP_OnUpgradeSuccess();
    }

    // TODO: 결과에 따른 토스트 메시지 표시
}

void UERNUpgradeMainWidget::OnInventorySlotChanged(const FInventoryItemEntry& Entry)
{
    // 인벤토리가 갱신(강화, 획득 등)되면 그제서야 UI 슬롯 목록을 갱신 (리플리케이션 지연으로 인한 낡은 데이터 표시 방지)
    PopulateInventorySlots();
}
