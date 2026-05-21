#include "UI/ERNUpgradeSlotWidget.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/Button.h"
#include "Inventory/Item/Data/EquipableItemDataAsset.h"
#include "Inventory/Item/Data/ERNItemTable.h"
#include "Inventory/Item/Data/ItemDataAssetBase.h"
#include "Engine/Texture2D.h"

void UERNUpgradeSlotWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (SlotButton)
    {
        SlotButton->OnClicked.AddDynamic(this, &UERNUpgradeSlotWidget::HandleButtonClicked);
    }
}

void UERNUpgradeSlotWidget::SetupSlot(int32 InSlotIndex, FName InItemID, const FERNItemTable* ItemRow)
{
    CachedSlotIndex = InSlotIndex;
    CachedItemID = InItemID;

    UE_LOG(LogTemp, Warning, TEXT("[DEBUG] SetupSlot 호출됨! SlotIndex: %d, ItemID: %s"), InSlotIndex, *InItemID.ToString());

    if (ItemRow)
    {
        FString DisplayNameStr = ItemRow->DisplayName.ToString();
        UE_LOG(LogTemp, Warning, TEXT("[DEBUG] ItemRow 정상 확인. DisplayName: '%s', 아이콘 유효성: %s"), 
            *DisplayNameStr, ItemRow->DataAsset.IsValid() ? TEXT("True") : TEXT("False"));

        if (ItemNameText)
        {
            ItemNameText->SetText(ItemRow->DisplayName);
            ItemNameText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
        }
        
        // 데이터 에셋 로드 및 아이콘 설정
        if (ItemIcon && !ItemRow->DataAsset.IsNull())
        {
            if (UItemDataAssetBase* Asset = ItemRow->DataAsset.LoadSynchronous())
            {
                if (UTexture2D* Tex = Asset->Icon.LoadSynchronous())
                {
                    ItemIcon->SetBrushFromTexture(Tex);
                    ItemIcon->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
                }
                else
                {
                    ItemIcon->SetVisibility(ESlateVisibility::Hidden);
                }
            }
        }
        
        // 등급 테두리 색상 설정
        if (GradeBorder || ItemNameText)
        {
            FLinearColor GradeColor = FLinearColor::White;
            switch (ItemRow->Grade)
            {
                case EItemGrade::Common: GradeColor = FLinearColor(0.8f, 0.8f, 0.8f, 1.f); break;
                case EItemGrade::Uncommon: GradeColor = FLinearColor(0.1f, 0.5f, 1.0f, 1.f); break;
                case EItemGrade::Rare: GradeColor = FLinearColor(0.6f, 0.1f, 1.0f, 1.f); break;
                case EItemGrade::Legendary: GradeColor = FLinearColor(1.0f, 0.8f, 0.1f, 1.f); break;
            }
            
            if (GradeBorder)
            {
                GradeBorder->SetColorAndOpacity(GradeColor);
            }
            
            if (ItemNameText)
            {
                ItemNameText->SetColorAndOpacity(FSlateColor(GradeColor));
            }
        }
    }
}

void UERNUpgradeSlotWidget::HandleButtonClicked()
{
    OnSlotClicked.Broadcast(CachedSlotIndex);
}
