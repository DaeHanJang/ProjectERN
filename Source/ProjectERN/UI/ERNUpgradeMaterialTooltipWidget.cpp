#include "UI/ERNUpgradeMaterialTooltipWidget.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"

void UERNUpgradeMaterialTooltipWidget::SetMaterialData(const FText& InMaterialName, EItemGrade InMaterialGrade,
    int32 InCurrentCount, UTexture2D* InMaterialIcon)
{
    // 단석 이름
    if (MaterialNameText)
    {
        MaterialNameText->SetText(InMaterialName);
    }

    // 수량 표시: "x1 (보유량)" 고정
    if (MaterialCountText)
    {
        MaterialCountText->SetText(FText::Format(
            NSLOCTEXT("Upgrade", "MatCountFormat", "x1 ({0})"),
            FText::AsNumber(InCurrentCount)));
        MaterialCountText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
    }

    // 등급 텍스트
    if (MaterialGradeText)
    {
        MaterialGradeText->SetText(GetGradeDisplayText(InMaterialGrade));
        MaterialGradeText->SetColorAndOpacity(FSlateColor(GetGradeColor(InMaterialGrade)));
    }

    // 단석 아이콘
    if (MaterialIconImage)
    {
        if (InMaterialIcon)
        {
            MaterialIconImage->SetBrushFromTexture(InMaterialIcon);
            MaterialIconImage->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
        }
        else
        {
            MaterialIconImage->SetVisibility(ESlateVisibility::Hidden);
        }
    }

    // 등급 테두리/필터 이미지 색상
    if (GradeFillterImage)
    {
        GradeFillterImage->SetColorAndOpacity(GetGradeColor(InMaterialGrade));
    }
}

void UERNUpgradeMaterialTooltipWidget::ClearMaterial()
{
    if (MaterialNameText) MaterialNameText->SetText(FText::FromString(TEXT("재료 없음")));
    if (MaterialCountText) MaterialCountText->SetText(FText::FromString(TEXT("-")));
    if (MaterialGradeText) MaterialGradeText->SetText(FText::GetEmpty());
    if (MaterialIconImage) MaterialIconImage->SetVisibility(ESlateVisibility::Hidden);
    if (GradeFillterImage) GradeFillterImage->SetColorAndOpacity(FLinearColor(0.3f, 0.3f, 0.3f, 1.f));
}

FLinearColor UERNUpgradeMaterialTooltipWidget::GetGradeColor(EItemGrade Grade) const
{
    switch (Grade)
    {
    case EItemGrade::Common:    return FLinearColor(0.65f, 0.65f, 0.65f, 1.0f);
    case EItemGrade::Uncommon:  return FLinearColor(0.2f, 1.0f, 1.0f, 1.0f);
    case EItemGrade::Rare:      return FLinearColor(0.2f, 0.05f, 1.0f, 1.0f);
    case EItemGrade::Legendary: return FLinearColor(1.0f, 0.265f, 0.0f, 1.0f);
    default:                    return FLinearColor(0.5f, 0.5f, 0.5f, 1.f);
    }
}

FText UERNUpgradeMaterialTooltipWidget::GetGradeDisplayText(EItemGrade Grade) const
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
