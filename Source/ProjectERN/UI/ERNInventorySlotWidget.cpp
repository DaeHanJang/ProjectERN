// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/ERNInventorySlotWidget.h"

#include "ERNInventoryWidget.h"
#include "UI/ERNInventoryDragDropOperation.h"
#include "Blueprint/DragDropOperation.h"
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

		// 마우스를 끌면 드래그 감지 시작 (짧은 클릭은 그대로 클릭으로 처리)
		return FReply::Handled().DetectDrag(TakeWidget(), EKeys::LeftMouseButton);
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UERNInventorySlotWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
	Super::NativeOnDragDetected(InGeometry, InMouseEvent, OutOperation);

	// 빈 슬롯은 드래그하지 않음
	if (!ItemImage || ItemImage->GetVisibility() == ESlateVisibility::Hidden)
	{
		return;
	}

	UERNInventoryDragDropOperation* Operation = NewObject<UERNInventoryDragDropOperation>();
	Operation->SourceSlotIndex = SlotIndex;
	Operation->Pivot = EDragPivot::CenterCenter; // 아이콘이 커서 중앙에 따라오도록

	// 실제 슬롯 아이콘이 화면에 그려진 크기 (없으면 슬롯 전체 크기로 폴백)
	FVector2D IconSize = ItemImage->GetCachedGeometry().GetLocalSize();
	if (IconSize.X <= 1.f || IconSize.Y <= 1.f)
	{
		IconSize = InGeometry.GetLocalSize();
	}

	// 현재 아이콘 브러시를 복제하고 ImageSize를 실제 크기로 지정
	FSlateBrush DragBrush = ItemImage->GetBrush();
	DragBrush.ImageSize = IconSize;

	UImage* DragVisual = NewObject<UImage>(this);
	DragVisual->SetBrush(DragBrush);
	Operation->DefaultDragVisual = DragVisual;

	OutOperation = Operation;
}

bool UERNInventorySlotWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
	UERNInventoryDragDropOperation* Operation = Cast<UERNInventoryDragDropOperation>(InOperation);
	if (!Operation)
	{
		return false;
	}

	if (Operation->SourceSlotIndex != SlotIndex)
	{
		// 이동/스왑/병합 처리는 상위 인벤토리 위젯이 담당
		OnSlotDropped.Broadcast(Operation->SourceSlotIndex, SlotIndex);
	}

	return true;
}

FReply UERNInventorySlotWidget::NativeOnMouseButtonDoubleClick(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		OnSlotDoubleClicked.Broadcast(SlotIndex);
		return FReply::Handled();
	}
	
	return Super::NativeOnMouseButtonDoubleClick(InGeometry, InMouseEvent);
}

void UERNInventorySlotWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);
	OnSlotHovered.Broadcast(SlotIndex);
}

void UERNInventorySlotWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);
	OnSlotUnhovered.Broadcast(SlotIndex);
}

void UERNInventorySlotWidget::SetInventorySlotImage(UTexture2D* NewTexture) const
{
	if (InventorySlotImage && NewTexture)
	{
		InventorySlotImage->SetBrushFromTexture(NewTexture);
	}
}

void UERNInventorySlotWidget::SetInventorySlotTint(FColor NewColor) const
{
	if (InventorySlotImage)
	{
		InventorySlotImage->SetBrushTintColor(NewColor);
	}
}

void UERNInventorySlotWidget::ClearItem()
{
	BackgroundTint = FColor::White;
	
	if (InventorySlotImage)
	{
		InventorySlotImage->SetBrushTintColor(BackgroundTint);
	}
	
	if (ItemImage)
	{
		ItemImage->SetBrushFromTexture(nullptr);
		ItemImage->SetVisibility(ESlateVisibility::Hidden);
	}
	
	if (ItemQuantityTextBlock)
	{
		ItemQuantityTextBlock->SetText(FText::GetEmpty());
		ItemQuantityTextBlock->SetVisibility(ESlateVisibility::Hidden);
	}

	if (ItemAbilityTextBlock)
	{
		ItemAbilityTextBlock->SetText(FText::GetEmpty());
		ItemAbilityTextBlock->SetVisibility(ESlateVisibility::Hidden);
	}
}

void UERNInventorySlotWidget::SetAbilityText(const FText& InText) const
{
	if (!ItemAbilityTextBlock)
	{
		return;
	}

	if (InText.IsEmpty())
	{
		ItemAbilityTextBlock->SetText(FText::GetEmpty());
		ItemAbilityTextBlock->SetVisibility(ESlateVisibility::Hidden);
	}
	else
	{
		ItemAbilityTextBlock->SetText(InText);
		ItemAbilityTextBlock->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
}

void UERNInventorySlotWidget::SetItem(UTexture2D* Icon, const int32 QuantityText, FColor Color)
{
	BackgroundTint = Color;
	
	if (InventorySlotImage)
	{
		InventorySlotImage->SetBrushTintColor(BackgroundTint);
	}
	
	if (ItemImage)
	{
		ItemImage->SetBrushFromTexture(Icon);
		ItemImage->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
	
	if (ItemQuantityTextBlock)
	{
		if (QuantityText <= 1)
		{
			ItemQuantityTextBlock->SetText(FText::GetEmpty());
		}
		else
		{
			ItemQuantityTextBlock->SetText(FText::AsNumber(QuantityText));
		}
		
		ItemQuantityTextBlock->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
}
