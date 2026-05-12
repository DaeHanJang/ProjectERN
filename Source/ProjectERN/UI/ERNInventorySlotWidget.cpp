// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/ERNInventorySlotWidget.h"

#include "ERNInventoryWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

UERNInventorySlotWidget::UERNInventorySlotWidget(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	SetIsFocusable(true);
}

FReply UERNInventorySlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// 마우스 왼쪽을 클릭했을 경우
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		// 슬롯 클릭 이벤트 브로드캐스트
		OnSlotClicked.Broadcast(SlotIndex);
		
		return FReply::Handled();
	}
	
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UERNInventorySlotWidget::SetInventorySlotImage(UTexture2D* NewTexture) const
{
	if (NewTexture)
	{
		InventorySlotImage->SetBrushFromTexture(NewTexture);
	}
}

void UERNInventorySlotWidget::ClearItem() const
{
	ItemImage->SetBrushFromTexture(nullptr);
	ItemQuantityTextBlock->SetText(FText::GetEmpty());
	
	ItemImage->SetVisibility(ESlateVisibility::Hidden);
	ItemQuantityTextBlock->SetVisibility(ESlateVisibility::Hidden);
}

void UERNInventorySlotWidget::SetItem(UTexture2D* Icon, const int32 QuantityText) const
{
	ItemImage->SetBrushFromTexture(Icon);
	if (QuantityText == 1)
	{
		ItemQuantityTextBlock->SetText(FText::GetEmpty());
	}
	else
	{
		ItemQuantityTextBlock->SetText(FText::AsNumber(QuantityText));
	}	
	
	ItemImage->SetVisibility(ESlateVisibility::Visible);
	ItemQuantityTextBlock->SetVisibility(ESlateVisibility::Visible);
}
