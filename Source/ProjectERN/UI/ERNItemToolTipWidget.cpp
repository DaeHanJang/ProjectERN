#include "ERNItemToolTipWidget.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Inventory/Item/Manager/ItemManagerSubsystem.h"
#include "Inventory/Item/Data/ItemDataAssetBase.h"
#include "Inventory/Item/Data/EquipableItemDataAsset.h"

void UERNItemToolTipWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 상점이 처음 열렸을 때 툴팁이 보이지 않도록 초기화
	SetVisibility(ESlateVisibility::Collapsed);
}

void UERNItemToolTipWidget::UpdateTooltip(FName ItemID, int32 ItemPrice)
{
	if (ItemID.IsNone()) return;

	if (UGameInstance* GI = GetGameInstance())
	{
		if (UItemManagerSubsystem* ItemSubsystem = GI->GetSubsystem<UItemManagerSubsystem>())
		{
			// 1. 데이터 테이블에서 아이템 기본 정보 가져오기 (이름)
			if (const FERNItemTable* ItemRow = ItemSubsystem->FindItemRow(ItemID))
			{
				if (ItemNameText)
				{
					ItemNameText->SetText(ItemRow->DisplayName);
				}

				FLinearColor TintColor = FLinearColor::White;
				switch (ItemRow->Grade)
				{
				case EItemGrade::Common:
					TintColor = FLinearColor(0.65f, 0.65f, 0.65f, 1.0f); // 회색
					break;
				case EItemGrade::Uncommon:
					TintColor = FLinearColor(0.2f, 1.0f, 1.0f, 1.0f); // 밝은 청녹색
					break;
				case EItemGrade::Rare:
					TintColor = FLinearColor(0.2f, 0.05f, 1.0f, 1.0f); // 진파랑-자색계열
					break;
				case EItemGrade::Legendary:
					TintColor = FLinearColor(1.0f, 0.265f, 0.0f, 1.0f); // 주황/금색
					break;
				default:
					TintColor = FLinearColor::White;
					break;
				}

				// 등급에 따른 배경 틴트 및 텍스트 색상 적용
				if (GradeBackgroundImage)
				{
					GradeBackgroundImage->SetColorAndOpacity(TintColor);
				}
				
				if (ItemNameText)
				{
					ItemNameText->SetColorAndOpacity(FSlateColor(TintColor));
				}

				if (DescriptionText)
				{
					DescriptionText->SetText(ItemRow->Description);
				}
			}

			// 2. 데이터 에셋에서 아이콘 가져오기 (UI용으로 동기 로드)
			// (캐싱되어 있다면 즉시 반환되며, 없을 경우 로드 대기가 발생할 수 있음)
			const UItemDataAssetBase* DataAsset = ItemSubsystem->LoadItemDataAssetSync(ItemID, EItemAssetLoadFlags::UI);
			if (DataAsset)
			{
				if (TooltipIcon)
				{
					if (UTexture2D* IconTex = DataAsset->Icon.LoadSynchronous())
					{
						TooltipIcon->SetBrushFromTexture(IconTex);
					}
				}

				if (const UEquipableItemDataAsset* EquipData = Cast<UEquipableItemDataAsset>(DataAsset))
				{
					if (DamageText)
					{
						DamageText->SetText(FText::AsNumber(EquipData->LightAttackDamage));
					}
					if (StatsPanel)
					{
						StatsPanel->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
					}
					if (AbilityPanel)
					{
						AbilityPanel->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
					}
				}
				else
				{
					if (DamageText)
					{
						DamageText->SetText(FText::FromString(TEXT("-")));
					}
					if (StatsPanel)
					{
						StatsPanel->SetVisibility(ESlateVisibility::Collapsed);
					}
					if (AbilityPanel)
					{
						AbilityPanel->SetVisibility(ESlateVisibility::Collapsed);
					}
				}
			}
			
			// 가격 정보 연동 (0원이면 인벤토리 등 가격이 필요 없는 상황으로 간주하고 UI 숨김 처리)
			if (PriceText)
			{
				if (ItemPrice > 0)
				{
					PriceText->SetText(FText::AsNumber(ItemPrice));
					if (UPanelWidget* ParentPanel = PriceText->GetParent())
					{
						ParentPanel->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
					}
					else
					{
						PriceText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
					}
				}
				else
				{
					if (UPanelWidget* ParentPanel = PriceText->GetParent())
					{
						ParentPanel->SetVisibility(ESlateVisibility::Collapsed);
					}
					else
					{
						PriceText->SetVisibility(ESlateVisibility::Collapsed);
					}
				}
			}
		}
	}
}
