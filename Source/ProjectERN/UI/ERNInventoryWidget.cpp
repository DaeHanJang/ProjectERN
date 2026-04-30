// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/ERNInventoryWidget.h"

#include "Character/Player/ERNPlayerController.h"
#include "Components/UniformGridPanel.h"
#include "UI/ERNInventorySlotWidget.h"
#include "InputCoreTypes.h"
#include "Character/Player/ProjectERNCharacter.h"
#include "Inventory/Components/ERNInventoryComponent.h"
#include "Inventory/Item/Data/ItemDataAssetBase.h"
#include "Inventory/Item/Manager/ItemManagerSubsystem.h"

UERNInventoryWidget::UERNInventoryWidget(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	bIsFocusable = true;
}

void UERNInventoryWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	SetVisibility(ESlateVisibility::Hidden);
	if (const AERNPlayerController* PC = GetOwningPlayer<AERNPlayerController>())
	{
		if (const AProjectERNCharacter* PlayerCharacter = Cast<AProjectERNCharacter>(PC->GetCharacter()))
		{
			if (UERNInventoryComponent* InventoryComponent = PlayerCharacter->GetInventoryComponent())
			{
				CreateSlot(InventoryComponent->GetMaxStackSize(), 4);
				
				InventoryComponent->OnInventorySlotChanged.AddDynamic(this, &UERNInventoryWidget::HandleInventorySlotChanged);
				
				for (const FInventoryItemEntry& Entry : InventoryComponent->GetInventoryItems())
				{
					HandleInventorySlotChanged(Entry);
				}
			}
		}
	}
}

FReply UERNInventoryWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::I || InKeyEvent.GetKey() == EKeys::Escape)
	{
		if (AERNPlayerController* PC = GetOwningPlayer<AERNPlayerController>())
		{
			PC->ToggleInventory();
			return FReply::Handled();
		}
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UERNInventoryWidget::CreateSlot(int32 MaxSlotSize, int32 ColumnCount)
{
	InventoryUniformGridPanel->ClearChildren();
	SlotWidgets.SetNum(MaxSlotSize);
	
	for (int32 i = 0; i < MaxSlotSize; ++i)
	{
		SlotWidgets[i] = CreateWidget<UERNInventorySlotWidget>(this, SlotWidgetClass);
		
		int32 Row = i / ColumnCount;
		int32 Col = i % ColumnCount;
		
		InventoryUniformGridPanel->AddChildToUniformGrid(SlotWidgets[i], Row, Col);
	}
}

void UERNInventoryWidget::HandleInventorySlotChanged(const FInventoryItemEntry& Entry)
{
	if (!SlotWidgets.IsValidIndex(Entry.SlotIndex))
	{
		return;
	}
	
	UItemManagerSubsystem* ItemManager = GetGameInstance()->GetSubsystem<UItemManagerSubsystem>();
	if (!ItemManager)
	{
		return;
	}
		
	const UItemDataAssetBase* ItemData = ItemManager->LoadItemDataAssetSync(Entry.ItemID, EItemAssetLoadFlags::UI);
	if (!ItemData)
	{
		return;
	}
	
	SlotWidgets[Entry.SlotIndex]->SetItem(ItemData->Icon.Get(), Entry.Quantity);
}
