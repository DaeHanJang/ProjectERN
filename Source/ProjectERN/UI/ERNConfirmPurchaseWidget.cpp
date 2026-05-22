// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/ERNConfirmPurchaseWidget.h"
#include "UI/ERNShopItemSlotWidget.h"
#include "Shop/Components/ERNShopComponent.h"
#include "Shop/Data/ERNShopTypes.h"
#include "Components/TextBlock.h"
#include "Inventory/Item/Data/ERNItemTable.h"
#include "Inventory/Item/Manager/ItemManagerSubsystem.h"

void UERNConfirmPurchaseWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	// 위젯이 생성될 때 포커스를 받아야 마우스 입력을 최우선으로 처리할 수 있음
	SetIsFocusable(true);
}

FReply UERNConfirmPurchaseWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// 좌클릭이나 우클릭이 발생하면 팝업 취소(No) 처리
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton || 
		InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		OnNoClicked();
		return FReply::Handled();
	}
	
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UERNConfirmPurchaseWidget::SetupPopup(const FERNShopItemData& InItemData)
{
	CachedItemData = InItemData;
	
	if (ItemNameText)
	{
		if (UItemManagerSubsystem* ItemMgr = GetWorld()->GetGameInstance()->GetSubsystem<UItemManagerSubsystem>())
		{
			if (const FERNItemTable* Row = ItemMgr->FindItemRow(CachedItemData.ItemID))
			{
				ItemNameText->SetText(Row->DisplayName);
				
				// 등급에 따른 텍스트 색상 적용
				FLinearColor GradeColor = FLinearColor::White;
				switch (Row->Grade)
				{
					case EItemGrade::Common: GradeColor = FLinearColor(0.65f, 0.65f, 0.65f, 1.0f); break;
					case EItemGrade::Uncommon: GradeColor = FLinearColor(0.2f, 1.0f, 1.0f, 1.0f); break;
					case EItemGrade::Rare: GradeColor = FLinearColor(0.2f, 0.05f, 1.0f, 1.0f); break;
					case EItemGrade::Legendary: GradeColor = FLinearColor(1.0f, 0.265f, 0.0f, 1.0f); break;
					default: GradeColor = FLinearColor::White; break;
				}
				ItemNameText->SetColorAndOpacity(FSlateColor(GradeColor));
			}
		}
	}

	if (PriceText)
	{
		FString FormattedPrice = FString::Printf(TEXT("%dG에 구매 하시겠습니까?"), CachedItemData.Price);
		PriceText->SetText(FText::FromString(FormattedPrice));
	}

	// 블루프린트 측 이벤트 호출하여 추가 텍스트 등 갱신
	BP_OnSetupPopup();
}

void UERNConfirmPurchaseWidget::OnYesClicked()
{
	if (ShopComponentRef)
	{
		// 수량은 기본 1개로 시도
		ShopComponentRef->TryPurchaseItem(CachedItemData.UniqueID, 1);
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
