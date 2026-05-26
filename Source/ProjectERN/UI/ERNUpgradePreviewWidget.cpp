#include "UI/ERNUpgradePreviewWidget.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"

void UERNUpgradePreviewWidget::SetPreviewData(const FText& DisplayName, EItemGrade Grade,
    int32 LightAtk, int32 HeavyAtk, bool bIsAfter, UTexture2D* WeaponIcon)
{
    if (ItemNameText)
        ItemNameText->SetText(DisplayName);

    if (GradeText)
    {
        GradeText->SetText(GetGradeDisplayText(Grade));
        GradeText->SetColorAndOpacity(FSlateColor(GetGradeColor(Grade)));
    }

    // 스탯 텍스트 색상: "강화 후" 모드에서는 초록색으로 표시
    FSlateColor StatColor = bIsAfter
        ? FSlateColor(FLinearColor(0.2f, 0.9f, 0.3f, 1.f))  // 초록색
        : FSlateColor(FLinearColor::White);

    if (LightAttackText)
    {
        LightAttackText->SetText(FText::AsNumber(LightAtk));
        if (bIsAfter) LightAttackText->SetColorAndOpacity(StatColor);
    }

    if (HeavyAttackText)
    {
        HeavyAttackText->SetVisibility(ESlateVisibility::Collapsed);
    }

    // 등급 테두리 색상
    if (GradeBorderImage)
    {
        GradeBorderImage->SetColorAndOpacity(GetGradeColor(Grade));
    }

    // 무기 아이콘
    if (WeaponIconImage)
    {
        if (WeaponIcon)
        {
            WeaponIconImage->SetBrushFromTexture(WeaponIcon);
            WeaponIconImage->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
        }
        else
        {
            WeaponIconImage->SetVisibility(ESlateVisibility::Hidden);
        }
    }
}

void UERNUpgradePreviewWidget::ClearPreview()
{
    if (ItemNameText)  ItemNameText->SetText(FText::FromString(TEXT("아이템을 선택하세요")));
    if (GradeText)     GradeText->SetText(FText::GetEmpty());
    if (LightAttackText) LightAttackText->SetText(FText::FromString(TEXT("-")));
    if (HeavyAttackText) HeavyAttackText->SetVisibility(ESlateVisibility::Collapsed);
    if (GradeBorderImage) GradeBorderImage->SetColorAndOpacity(FLinearColor(0.3f, 0.3f, 0.3f, 1.f));
    if (WeaponIconImage) WeaponIconImage->SetVisibility(ESlateVisibility::Hidden);
}

FLinearColor UERNUpgradePreviewWidget::GetGradeColor(EItemGrade Grade) const
{
    switch (Grade)
    {
    case EItemGrade::Common:    return FLinearColor(0.65f, 0.65f, 0.65f, 1.0f);  // 회색
    case EItemGrade::Uncommon:  return FLinearColor(0.2f, 1.0f, 1.0f, 1.0f);     // 밝은 청녹색
    case EItemGrade::Rare:      return FLinearColor(0.2f, 0.05f, 1.0f, 1.0f);    // 진파랑-자색계열
    case EItemGrade::Legendary: return FLinearColor(1.0f, 0.265f, 0.0f, 1.0f);   // 주황/금색
    default:                    return FLinearColor(0.5f, 0.5f, 0.5f, 1.f);
    }
}

FText UERNUpgradePreviewWidget::GetGradeDisplayText(EItemGrade Grade) const
{
    switch (Grade)
    {
    case EItemGrade::Common:    return FText::FromString(TEXT("Common"));
    case EItemGrade::Uncommon:  return FText::FromString(TEXT("Uncommon"));
    case EItemGrade::Rare:      return FText::FromString(TEXT("Rare"));
    case EItemGrade::Legendary: return FText::FromString(TEXT("Legendary"));
    default:                    return FText::FromString(TEXT("Unknown"));
    }
}
