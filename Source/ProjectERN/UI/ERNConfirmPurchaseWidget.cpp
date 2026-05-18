// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/ERNConfirmPurchaseWidget.h"
#include "UI/ERNShopItemSlotWidget.h"
#include "Shop/Components/ERNShopComponent.h"
#include "Shop/Data/ERNShopTypes.h"

void UERNConfirmPurchaseWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

void UERNConfirmPurchaseWidget::SetupPopup(const FERNShopItemData& InItemData)
{
	CachedItemData = InItemData;
	
	// 블루프린트 측 이벤트 호출하여 텍스트 등 갱신
	BP_OnSetupPopup();
}

void UERNConfirmPurchaseWidget::OnYesClicked()
{
	if (ShopComponentRef)
	{
		// 수량은 기본 1개로 시도
		ShopComponentRef->TryPurchaseItem(CachedItemData.ItemID, 1);
	}
	else
	{
		UE_LOG(LogShopProvider, Warning, TEXT("[ERNConfirmPurchaseWidget] ShopComponentRef가 유효하지 않아 구매 시도를 할 수 없습니다."));
	}

	// Yes를 눌렀으므로 팝업 닫기
	RemoveFromParent();
}

void UERNConfirmPurchaseWidget::OnNoClicked()
{
	// 취소 시 버튼을 다시 활성화
	if (OwnerSlotWidget)
	{
		OwnerSlotWidget->SetPurchaseButtonEnabled(true);
	}
	
	// No를 눌렀으므로 아무 동작 없이 팝업 닫기
	RemoveFromParent();
}
