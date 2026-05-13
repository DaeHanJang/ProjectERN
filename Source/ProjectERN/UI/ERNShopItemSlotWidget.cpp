#include "ERNShopItemSlotWidget.h"

void UERNShopItemSlotWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);

	// 호버 이벤트 발생 시, 현재 슬롯이 가지고 있는 아이템 데이터 브로드캐스트
	if (OnSlotHovered.IsBound())
	{
		OnSlotHovered.Broadcast(ItemData);
	}
}

void UERNShopItemSlotWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);

	// 언호버 이벤트 알림
	if (OnSlotUnhovered.IsBound())
	{
		OnSlotUnhovered.Broadcast();
	}
}
