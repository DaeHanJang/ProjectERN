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
					case EItemGrade::Common: GradeColor = FLinearColor(0.8f, 0.8f, 0.8f, 1.f); break;
					case EItemGrade::Uncommon: GradeColor = FLinearColor(0.2f, 0.5f, 1.0f, 1.f); break;
					case EItemGrade::Rare: GradeColor = FLinearColor(0.6f, 0.2f, 0.9f, 1.f); break;
					case EItemGrade::Legendary: GradeColor = FLinearColor(1.0f, 0.85f, 0.0f, 1.f); break;
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
