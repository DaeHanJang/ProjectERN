// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/ERNQuickSlotWidget.h"
#include "UI/ERNInventorySlotWidget.h"
#include "Character/Player/ERNPlayerController.h"
#include "Character/Player/ProjectERNCharacter.h"
#include "Inventory/Components/ERNEquipmentComponent.h"
#include "Inventory/Item/Data/ItemDataAssetBase.h"
#include "Inventory/Item/Manager/ItemManagerSubsystem.h"
#include "UI/ERNUIManagerSubsystem.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"

UERNQuickSlotWidget::UERNQuickSlotWidget(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

void UERNQuickSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	RefreshFromCurrentCharacter();

	if (const ULocalPlayer* LocalPlayer = GetOwningLocalPlayer())
	{
		if (UERNUIManagerSubsystem* UIManager = LocalPlayer->GetSubsystem<UERNUIManagerSubsystem>())
		{
			UIManager->OnUIStateChanged.AddUniqueDynamic(this, &UERNQuickSlotWidget::HandleUIStateChanged);
			HandleUIStateChanged(UIManager->GetActiveUIType()); // 초기화
		}
	}
}

void UERNQuickSlotWidget::NativeDestruct()
{
	if (const ULocalPlayer* LocalPlayer = GetOwningLocalPlayer())
	{
		if (UERNUIManagerSubsystem* UIManager = LocalPlayer->GetSubsystem<UERNUIManagerSubsystem>())
		{
			UIManager->OnUIStateChanged.RemoveDynamic(this, &UERNQuickSlotWidget::HandleUIStateChanged);
		}
	}

	UnbindFromCurrentComponent();
	
	Super::NativeDestruct();
}

void UERNQuickSlotWidget::HandleUIStateChanged(EERNUIType UIType)
{
	if (UIType != EERNUIType::None)
	{
		SetVisibility(ESlateVisibility::Hidden);
	}
	else
	{
		SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
}

void UERNQuickSlotWidget::RefreshFromCurrentCharacter()
{
	UnbindFromCurrentComponent();

	if (UERNEquipmentComponent* EquipmentComponent = GetEquipmentComponent())
	{
		BoundEquipmentComponent = EquipmentComponent;
		
		EquipmentComponent->OnEquipmentSlotChanged.AddUniqueDynamic(this, &UERNQuickSlotWidget::UpdateEquipableSlot);
		EquipmentComponent->OnConsumableSlotChanged.AddUniqueDynamic(this, &UERNQuickSlotWidget::UpdateConsumableSlot);
		
		UpdateEquipableSlot(EquipmentComponent->EquipableSlot);
		UpdateConsumableSlot(EquipmentComponent->ConsumableSlot);
	}
	else
	{
		BoundEquipmentComponent = nullptr;
		
		if (EquipableSlotWidget)
		{
			EquipableSlotWidget->ClearItem();
			EquipableSlotWidget->SetInventorySlotImage(BasicSlotImage.Get());
		}
		if (ConsumableSlotWidget)
		{
			ConsumableSlotWidget->ClearItem();
			ConsumableSlotWidget->SetInventorySlotImage(BasicConsumableSlotImage.Get());
		}
	}
}

void UERNQuickSlotWidget::UnbindFromCurrentComponent()
{
	if (UERNEquipmentComponent* EquipmentComponent = BoundEquipmentComponent.Get())
	{
		EquipmentComponent->OnEquipmentSlotChanged.RemoveDynamic(this, &UERNQuickSlotWidget::UpdateEquipableSlot);
		EquipmentComponent->OnConsumableSlotChanged.RemoveDynamic(this, &UERNQuickSlotWidget::UpdateConsumableSlot);
	}
	
	BoundEquipmentComponent = nullptr;
}

UERNEquipmentComponent* UERNQuickSlotWidget::GetEquipmentComponent() const
{
	if (const AERNPlayerController* PC = GetOwningPlayer<AERNPlayerController>())
	{
		if (const AProjectERNCharacter* PlayerCharacter = Cast<AProjectERNCharacter>(PC->GetCharacter()))
		{
			return PlayerCharacter->GetEquipmentComponent();
		}
	}
	return nullptr;
}

void UERNQuickSlotWidget::UpdateEquipableSlot(const FInventoryItemEntry& Entry)
{
	if (!EquipableSlotWidget) return;

	if (Entry.GetItemID().IsNone() || Entry.GetItemID() == NAME_None || Entry.GetQuantity() <= 0)
	{
		EquipableSlotWidget->ClearItem();
		EquipableSlotWidget->SetInventorySlotImage(BasicSlotImage.Get());
		EquipableSlotWidget->SetInventorySlotTint(FColor::White);
		return;
	}
	
	if (UItemManagerSubsystem* ItemManager = GetGameInstance()->GetSubsystem<UItemManagerSubsystem>())
	{
		TWeakObjectPtr<UERNQuickSlotWidget> WeakThis(this);
		const FName ItemID = Entry.GetItemID();
		const int32 Quantity = Entry.GetQuantity();
		EItemGrade Grade = ItemManager->FindItemRow(Entry.GetItemID())->Grade;
		
		ItemManager->PreloadItemDataAssetAsync(Entry.GetItemID(), EItemAssetLoadFlags::UI, 
			FOnItemDataAssetLoaded::CreateLambda([WeakThis, ItemID, Quantity, Grade](const UItemDataAssetBase* ItemData)
			{
				if (!WeakThis.IsValid() || !ItemData)
				{
					return;
				}
				
				const UERNEquipmentComponent* EquipmentComponent = WeakThis->GetEquipmentComponent();
				if (!EquipmentComponent)
				{
					return;
				}

				const FInventoryItemEntry& CurrentEntry = EquipmentComponent->EquipableSlot;
				if (CurrentEntry.GetItemID() != ItemID || CurrentEntry.GetQuantity() != Quantity)
				{
					return;
				}
				
				WeakThis->EquipableSlotWidget->SetItem(ItemData->Icon.Get(), Quantity, WeakThis->ItemGradeByColor(Grade));
			}
		));
	}
}

void UERNQuickSlotWidget::UpdateConsumableSlot(const FInventoryItemEntry& Entry)
{
	if (!ConsumableSlotWidget) return;

	if (Entry.GetItemID().IsNone() || Entry.GetItemID() == NAME_None || Entry.GetQuantity() <= 0)
	{
		ConsumableSlotWidget->ClearItem();
		ConsumableSlotWidget->SetInventorySlotImage(BasicConsumableSlotImage.Get());
		ConsumableSlotWidget->SetInventorySlotTint(FColor::White);
		return;
	}
	
	if (UItemManagerSubsystem* ItemManager = GetGameInstance()->GetSubsystem<UItemManagerSubsystem>())
	{
		TWeakObjectPtr<UERNQuickSlotWidget> WeakThis(this);
		const FName ItemID = Entry.GetItemID();
		const int32 Quantity = Entry.GetQuantity();
		EItemGrade Grade = ItemManager->FindItemRow(Entry.GetItemID())->Grade;
		
		ItemManager->PreloadItemDataAssetAsync(Entry.GetItemID(), EItemAssetLoadFlags::UI,
			FOnItemDataAssetLoaded::CreateLambda([WeakThis, ItemID, Quantity, Grade](const UItemDataAssetBase* ItemData)
			{
				if (!WeakThis.IsValid() || !ItemData)
				{
					return;
				}
				
				const UERNEquipmentComponent* EquipmentComponent = WeakThis->GetEquipmentComponent();
				if (!EquipmentComponent)
				{
					return;
				}

				const FInventoryItemEntry& CurrentEntry = EquipmentComponent->ConsumableSlot;
				if (CurrentEntry.GetItemID() != ItemID || CurrentEntry.GetQuantity() != Quantity)
				{
					return;
				}
				
				WeakThis->ConsumableSlotWidget->SetItem(ItemData->Icon.Get(), Quantity, WeakThis->ItemGradeByColor(Grade));
			}
		));
	}
}

FColor UERNQuickSlotWidget::ItemGradeByColor(EItemGrade Grade)
{
	FColor Color;
	switch (Grade)
	{
	case EItemGrade::Uncommon:
		Color = FColor::Cyan;
		break;
	case EItemGrade::Rare:
		Color =  FColor::Purple;
		break;
	case EItemGrade::Legendary:
		Color =  FColor::Orange;
		break;
	default:
		Color =  FColor::White;
		break;
	}
	
	return Color;
}
