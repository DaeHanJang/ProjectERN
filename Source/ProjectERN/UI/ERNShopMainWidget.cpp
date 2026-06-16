// 상점 메인 위젯 - 소지 골드 표시 및 실시간 갱신 구현

#include "UI/ERNShopMainWidget.h"
#include "Components/TextBlock.h"
#include "Character/Player/ProjectERNCharacter.h"
#include "GAS/ERNAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Kismet/GameplayStatics.h"
#include "UI/ERNItemToolTipWidget.h"
#include "Inventory/Item/Manager/ItemManagerSubsystem.h"
#include "Inventory/Item/Data/ItemDataAssetBase.h"
#include "Inventory/Item/Data/ERNItemEnums.h"
#include "Inventory/Components/ERNEquipmentComponent.h"
#include "Shop/Components/ERNShopComponent.h"
#include "UI/ERNShopItemSlotWidget.h"
#include "Components/PanelWidget.h"

UERNShopMainWidget::UERNShopMainWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsFocusable(true);
}

void UERNShopMainWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 로컬 플레이어의 캐릭터에서 AttributeSet과 ASC를 가져와 골드 변경 감지를 등록합니다.
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (!PC) return;

	AProjectERNCharacter* PlayerChar = Cast<AProjectERNCharacter>(PC->GetPawn());
	if (!PlayerChar) return;

	UAbilitySystemComponent* ASC = PlayerChar->GetAbilitySystemComponent();
	if (!ASC) return;

	CachedASC = ASC;

	// Gold 어트리뷰트 변경 감지 등록
	GoldChangedDelegateHandle = ASC->GetGameplayAttributeValueChangeDelegate(
		UERNAttributeSet::GetGoldAttribute()
	).AddUObject(this, &UERNShopMainWidget::OnGoldChanged);

	// 상점 컴포넌트 이벤트 바인딩
	if (UERNShopComponent* ShopComp = PlayerChar->GetShopComponent())
	{
		ShopComp->OnPurchaseResult.AddUniqueDynamic(this, &UERNShopMainWidget::OnPurchaseResultReceived);
	}

	// 최초 1회 골드 표시
	RefreshGoldDisplay();
}

void UERNShopMainWidget::NativeDestruct()
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (PC && PC->GetPawn())
	{
		if (AProjectERNCharacter* PlayerChar = Cast<AProjectERNCharacter>(PC->GetPawn()))
		{
			if (UERNShopComponent* ShopComp = PlayerChar->GetShopComponent())
			{
				ShopComp->OnPurchaseResult.RemoveDynamic(this, &UERNShopMainWidget::OnPurchaseResultReceived);
			}
		}
	}

	// 위젯이 파괴될 때 델리게이트 해제 (메모리 누수 방지)
	if (CachedASC)
	{
		CachedASC->GetGameplayAttributeValueChangeDelegate(
			UERNAttributeSet::GetGoldAttribute()
		).Remove(GoldChangedDelegateHandle);
		CachedASC = nullptr;
	}

	Super::NativeDestruct();
}

void UERNShopMainWidget::OnGoldChanged(const FOnAttributeChangeData& Data)
{
	// 골드 값이 변경될 때마다 자동 호출됩니다.
	RefreshGoldDisplay();
}

void UERNShopMainWidget::RefreshGoldDisplay()
{
	if (!GoldText) return;

	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (!PC) return;

	AProjectERNCharacter* PlayerChar = Cast<AProjectERNCharacter>(PC->GetPawn());
	if (!PlayerChar) return;

	UERNAttributeSet* AttributeSet = PlayerChar->GetAttributeSet();
	if (!AttributeSet) return;

	int32 CurrentGold = FMath::FloorToInt(AttributeSet->GetGold());
	GoldText->SetText(FText::AsNumber(CurrentGold));
}

void UERNShopMainWidget::ClearShopTooltip()
{
	if (WBP_ItemToolTip)
	{
		WBP_ItemToolTip->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (WBP_EquippedItemToolTip)
	{
		WBP_EquippedItemToolTip->SetVisibility(ESlateVisibility::Collapsed);
	}
}

FNavigationReply UERNShopMainWidget::NativeOnNavigation(const FGeometry& MyGeometry, const FNavigationEvent& InNavigationEvent, const FNavigationReply& InDefaultReply)
{
	// 패드나 키보드 내비게이션(방향키)이 발생했을 때 호출
	if (ItemListBox && ItemListBox->GetChildrenCount() > 0)
	{
		bool bHasSlotFocus = false;
		for (int32 i = 0; i < ItemListBox->GetChildrenCount(); ++i)
		{
			if (ItemListBox->GetChildAt(i)->HasAnyUserFocus())
			{
				bHasSlotFocus = true;
				break;
			}
		}

		// 만약 현재 슬롯들 중 어느 곳에도 포커스가 없다면
		// (즉, 상점이 막 켜졌거나 빈 곳에 포커스가 있는 상태에서 방향키를 누름)
		if (!bHasSlotFocus)
		{
			UWidget* FirstChild = ItemListBox->GetChildAt(0);
			if (FirstChild)
			{
				TSharedPtr<SWidget> SafeWidget = FirstChild->GetCachedWidget();
				if (SafeWidget.IsValid())
				{
					// 0번째 슬롯(Slate 위젯)으로 포커스를 명시적으로 이동시킵니다.
					return FNavigationReply::Explicit(SafeWidget);
				}
			}
		}
	}

	return Super::NativeOnNavigation(MyGeometry, InNavigationEvent, InDefaultReply);
}

void UERNShopMainWidget::UpdateShopTooltip(const FERNShopItemData& ShopItemData)
{
	if (WBP_ItemToolTip)
	{
		if (ShopItemData.ItemID.IsNone() || ShopItemData.ItemID == NAME_None)
		{
			ClearShopTooltip();
			return;
		}

		FItemRuntimeState TooltipState = ShopItemData.ItemState;
		if (TooltipState.GetItemID().IsNone())
		{
			TooltipState.SetItemID(ShopItemData.ItemID);
		}

		WBP_ItemToolTip->UpdateTooltipWithState(TooltipState, ShopItemData.Price);
		WBP_ItemToolTip->SetVisibility(ESlateVisibility::HitTestInvisible);
		
		bool bIsEquipment = false;
		FName EquippedItemID = NAME_None;
		if (UGameInstance* GI = GetGameInstance())
		{
			// ItemManagerSubsystem과 ERNItemEnums는 프로젝트 구조에 따라 헤더 포함이 필요할 수 있습니다.
			// 상단에 이미 Include가 되어있는지 확인, 아니면 런타임에 Cast로 처리하기 위해 생략하지 않고 Include 추가.
			if (UItemManagerSubsystem* ItemSubsystem = GI->GetSubsystem<UItemManagerSubsystem>())
			{
				if (const FERNItemTable* ItemRow = ItemSubsystem->FindItemRow(ShopItemData.ItemID))
				{
					if (ItemRow->ItemType == EItemType::Equipable)
					{
						bIsEquipment = true;
					}
				}
			}
		}
		
		if (bIsEquipment)
		{
			if (WBP_EquippedItemToolTip)
			{
				FItemRuntimeState EquippedItemState;
				APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
				if (PC && PC->GetPawn())
				{
					if (UERNEquipmentComponent* EquipComp = PC->GetPawn()->FindComponentByClass<UERNEquipmentComponent>())
					{
						EquippedItemID = EquipComp->EquipableSlot.GetItemID();
						EquippedItemState = EquipComp->EquipableSlot.GetItemRuntimeState();
					}
				}
				
				if (EquippedItemState.IsValid())
				{
					WBP_ItemToolTip->SetTooltipStateText(FText::FromString(TEXT("비교")));
					WBP_EquippedItemToolTip->UpdateTooltipWithState(EquippedItemState, 0);
					WBP_EquippedItemToolTip->SetTooltipStateText(FText::FromString(TEXT("장착 중")));
					WBP_EquippedItemToolTip->SetVisibility(ESlateVisibility::HitTestInvisible);
				}
				else
				{
					WBP_ItemToolTip->SetTooltipStateText(FText::GetEmpty());
					WBP_EquippedItemToolTip->SetVisibility(ESlateVisibility::Collapsed);
				}
			}
		}
		else
		{
			WBP_ItemToolTip->SetTooltipStateText(FText::GetEmpty());
			if (WBP_EquippedItemToolTip) WBP_EquippedItemToolTip->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void UERNShopMainWidget::OnPurchaseResultReceived(const FERNShopTransaction& Transaction)
{
	if (Transaction.Result != EERNTransactionResult::Success)
	{
		// 블루프린트에 바인딩된 랩박스(ItemListBox)를 순회하며 일치하는 슬롯 탐색
		if (ItemListBox)
		{
			for (int32 i = 0; i < ItemListBox->GetChildrenCount(); ++i)
			{
				UWidget* ChildWidget = ItemListBox->GetChildAt(i);
				if (UERNShopItemSlotWidget* SlotWidget = Cast<UERNShopItemSlotWidget>(ChildWidget))
				{
					// 구매 실패한 고유 아이디와 슬롯의 고유 아이디가 일치하면 피드백 호출
					if (SlotWidget->ItemData.UniqueID == Transaction.UniqueID)
					{
						SlotWidget->BP_PlayFailureFeedback(Transaction.Result);
						// 결제 실패 시 비활성화(반투명)된 버튼을 다시 원상복구
						SlotWidget->SetPurchaseButtonEnabled(true);
						break;
					}
				}
			}
		}
	}
}
