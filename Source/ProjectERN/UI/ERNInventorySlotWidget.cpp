// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/ERNInventorySlotWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"

void UERNInventorySlotWidget::ClearItem() const
{
	ItemImage->SetBrushFromTexture(nullptr);
	ItemQuantityTextBlock->SetText(FText::GetEmpty());
	
	ItemImage->SetVisibility(ESlateVisibility::Hidden);
	ItemQuantityTextBlock->SetVisibility(ESlateVisibility::Hidden);
}

void UERNInventorySlotWidget::SetItem(UTexture2D* Icon, int32 QuantityText) const
{
	ItemImage->SetBrushFromTexture(Icon);
	ItemQuantityTextBlock->SetText(FText::AsNumber(QuantityText));
	
	ItemImage->SetVisibility(ESlateVisibility::Visible);
	ItemQuantityTextBlock->SetVisibility(ESlateVisibility::Visible);
}
