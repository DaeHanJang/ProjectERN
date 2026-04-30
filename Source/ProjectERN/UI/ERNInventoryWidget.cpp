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
	SetIsFocusable(true);
}

void UERNInventoryWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	SetVisibility(ESlateVisibility::Hidden);
	
	if (UERNInventoryComponent* InventoryComponent = GetInventoryComponent())
	{
		CreateSlot(InventoryComponent->GetMaxStackSize(), ColumnSize);
		
		InventoryComponent->OnInventorySlotChanged.AddDynamic(this, &UERNInventoryWidget::HandleInventorySlotChanged);
		
		for (const FInventoryItemEntry& Entry : InventoryComponent->GetInventoryItems())
		{
			HandleInventorySlotChanged(Entry);
		}
	}
}

FReply UERNInventoryWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	// 키보드 I나 Esc를 누를 경우
	if (InKeyEvent.GetKey() == EKeys::I || InKeyEvent.GetKey() == EKeys::Escape)
	{
		if (AERNPlayerController* PC = GetOwningPlayer<AERNPlayerController>())
		{
			// 인벤토리 위젯 숨기기
			PC->ToggleInventory();
		
			return FReply::Handled();
		}
	}
	
	UERNInventoryComponent* InventoryComponent = GetInventoryComponent();
	if (!InventoryComponent)
	{
		return FReply::Handled();
	}
	
	// 키보드 G를 누를 경우
	if (InKeyEvent.GetKey() == EKeys::G && FocusSlotIndex != -1)
	{
		InventoryComponent->Server_RemoveItem(FocusSlotIndex, 1);
		SetFocusSlotIndex(-1);
			
		return FReply::Handled();
	}
	// 인벤토리 네비게이션
	if (InKeyEvent.GetKey() == EKeys::W && FocusSlotIndex != -1)
	{
		const int32 UpIndex = (FocusSlotIndex - ColumnSize < 0) ? 
		FocusSlotIndex + ColumnSize * (((FocusSlotIndex - ColumnSize) * -1 + InventoryComponent->GetMaxStackSize() - 1) / ColumnSize - 1)
		: FocusSlotIndex - ColumnSize;
		SetFocusSlotIndex(UpIndex);
		
		return FReply::Handled();
	}
	if (InKeyEvent.GetKey() == EKeys::S && FocusSlotIndex != -1)
	{
		const int32 DownIndex = FocusSlotIndex + ColumnSize >= InventoryComponent->GetMaxStackSize() ?
		(FocusSlotIndex + ColumnSize) % ColumnSize : FocusSlotIndex + ColumnSize;
		SetFocusSlotIndex(DownIndex);
		
		return FReply::Handled();
	}
	if (InKeyEvent.GetKey() == EKeys::A && FocusSlotIndex != -1)
	{
		const int32 PreviousIndex = (FocusSlotIndex - 1 < 0) ? 
		InventoryComponent->GetMaxStackSize() - 1 : FocusSlotIndex - 1;
		SetFocusSlotIndex(PreviousIndex);
		
		return FReply::Handled();
	}
	if (InKeyEvent.GetKey() == EKeys::D && FocusSlotIndex != -1)
	{
		const int32 NextIndex = (FocusSlotIndex + 1) % InventoryComponent->GetMaxStackSize();
		SetFocusSlotIndex(NextIndex);
		
		return FReply::Handled();
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
		
		if (SlotWidgets[i])
		{
			SlotWidgets[i]->SetSlotIndex(i);
			SlotWidgets[i]->OnSlotClicked.AddDynamic(this, &UERNInventoryWidget::SetFocusSlotIndex);
		}
	}
}

void UERNInventoryWidget::HandleInventorySlotChanged(const FInventoryItemEntry& Entry)
{
	if (!SlotWidgets.IsValidIndex(Entry.SlotIndex))
	{
		return;
	}
	
	if (Entry.Quantity <= 0)
	{
		SlotWidgets[Entry.SlotIndex]->ClearItem();
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

void UERNInventoryWidget::SetFocusSlotIndex(const int32 NewIndex)
{
	if (FocusSlotIndex != -1)
	{
		SlotWidgets[FocusSlotIndex]->SetInventorySlotImage(BasicSlotImage.Get());
	}
	
	if (NewIndex != -1)
	{
		SlotWidgets[NewIndex]->SetInventorySlotImage(FocusSlotImage.Get());
	}
	
	FocusSlotIndex = NewIndex;
}

UERNInventoryComponent* UERNInventoryWidget::GetInventoryComponent() const
{
	if (AERNPlayerController* PC = GetOwningPlayer<AERNPlayerController>())
	{
		if (const AProjectERNCharacter* PlayerCharacter = Cast<AProjectERNCharacter>(PC->GetCharacter()))
		{
			return PlayerCharacter->GetInventoryComponent();
		}
	}
	
	return nullptr;
}
