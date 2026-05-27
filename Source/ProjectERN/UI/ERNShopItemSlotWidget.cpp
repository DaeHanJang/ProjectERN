#include "UI/ERNShopItemSlotWidget.h"
#include "UI/ERNConfirmPurchaseWidget.h"
#include "Character/Player/ProjectERNCharacter.h"
#include "Shop/Components/ERNShopComponent.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"
#include "GAS/ERNAttributeSet.h"
#include "UI/ERNUIManagerSubsystem.h"
#include "Components/Image.h"

void UERNShopItemSlotWidget::OnPurchaseButtonClicked()
{
	if (!ConfirmPurchaseWidgetClass)
	{
		// 클래스가 설정되지 않았으면 무시
		return;
	}

	// 팝업 생성
	UERNConfirmPurchaseWidget* Popup = CreateWidget<UERNConfirmPurchaseWidget>(GetOwningPlayer(), ConfirmPurchaseWidgetClass);

	if (!Popup) return;

	// ShopComponent 참조 전달
	if (AProjectERNCharacter* Character = Cast<AProjectERNCharacter>(GetOwningPlayerPawn()))
	{
		Popup->ShopComponentRef = Character->GetShopComponent();
	}

	// 아이템 데이터 전달 및 팝업 표시
	Popup->OwnerSlotWidget = this;
	Popup->SetupPopup(ItemData);
	Popup->AddToViewport(200); // 상점 UI(100)보다 높은 ZOrder로 표시

	// UI 매니저에 팝업 등록 (기존 팝업 자동 닫힘)
	if (ULocalPlayer* LocalPlayer = GetOwningLocalPlayer())
	{
		if (UERNUIManagerSubsystem* UIManager = LocalPlayer->GetSubsystem<UERNUIManagerSubsystem>())
		{
			UIManager->RegisterConfirmPurchasePopup(Popup);
		}
	}

	// 버튼 즉시 비활성화 (중복 클릭 방지)
	SetPurchaseButtonEnabled(false);
}

void UERNShopItemSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (HoverImage)
	{
		HoverImage->SetVisibility(ESlateVisibility::Hidden);
	}

	// 위젯이 생성될 때 텍스트 UI 갱신
	RefreshSlotUI();
}

void UERNShopItemSlotWidget::RefreshSlotUI()
{
	if (PriceText)
	{
		PriceText->SetText(FText::AsNumber(ItemData.Price));

		// 골드 부족 시 가격 텍스트 빨간색 표시 (Phase 4 상태 표시 연동)
		APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
		if (PC)
		{
			if (AProjectERNCharacter* PlayerChar = Cast<AProjectERNCharacter>(PC->GetPawn()))
			{
				if (UERNAttributeSet* AttributeSet = PlayerChar->GetAttributeSet())
				{
					if (AttributeSet->GetGold() < ItemData.Price)
					{
						PriceText->SetColorAndOpacity(FSlateColor(FLinearColor::Red));
					}
					else
					{
						PriceText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
					}
				}
			}
		}
	}

	if (StockText)
	{
		if (ItemData.StockCount == -1)
		{
			StockText->SetText(FText::FromString(TEXT("∞"))); // 무제한
		}
		else
		{
			StockText->SetText(FText::AsNumber(ItemData.StockCount));
		}
	}

	// 재고가 0이면 구매 버튼 비활성화 (Phase 4 상태 표시 연동)
	if (ItemData.StockCount == 0)
	{
		SetPurchaseButtonEnabled(false);
	}
	else
	{
		SetPurchaseButtonEnabled(true);
	}
}

void UERNShopItemSlotWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);

	if (HoverImage)
	{
		HoverImage->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}

	// 호버 이벤트 발생 시, 현재 슬롯이 가지고 있는 아이템 데이터 브로드캐스트
	if (OnSlotHovered.IsBound())
	{
		OnSlotHovered.Broadcast(ItemData);
	}
}

void UERNShopItemSlotWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);

	if (HoverImage)
	{
		HoverImage->SetVisibility(ESlateVisibility::Hidden);
	}

	// 언호버 이벤트 알림
	if (OnSlotUnhovered.IsBound())
	{
		OnSlotUnhovered.Broadcast();
	}
}
