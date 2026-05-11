// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/ERNInventoryWidget.h"

#include "ERNSlideWidget.h"
#include "Character/Player/ERNPlayerController.h"
#include "Components/UniformGridPanel.h"
#include "UI/ERNInventorySlotWidget.h"
#include "InputCoreTypes.h"
#include "Character/Player/ProjectERNCharacter.h"
#include "Inventory/Components/ERNEquipmentComponent.h"
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
	
	// 현재 캐릭터 컴포넌트에 이벤트 바인딩
	RefreshFromCurrentCharacter();
	
	// 슬라이드 위젯 확인, 취소 버튼 클릭 이벤트 구독
	if (WBP_SlideWidget)
	{
		WBP_SlideWidget->OnConfirmButtonClicked.AddUniqueDynamic(this, &UERNInventoryWidget::UpdateSlideWidget);
		WBP_SlideWidget->OnCancelButtonClicked.AddUniqueDynamic(this, &UERNInventoryWidget::UpdateSlideWidget);
	}
}

void UERNInventoryWidget::RefreshFromCurrentCharacter()
{
	// 구독된 이벤트가 있다면 해제
	UnbindFromCurrentComponent();
	
	// 활성화 슬롯 초기화
	InitFocusSlotIndex();
	
	UERNInventoryComponent* InventoryComponent = GetInventoryComponent();
	if (InventoryComponent)
	{
		// 현재 캐릭터의 인벤토리 컴포넌트 설정
		BoundInventoryComponent = InventoryComponent;
		
		// 슬롯 생성
		CreateSlot(InventoryComponent->GetMaxStackSize(), ColumnSize);
		
		// 인벤토리 갱신 이벤트 구독
		InventoryComponent->OnInventorySlotChanged.AddUniqueDynamic(this, &UERNInventoryWidget::UpdateInventorySlot);
		
		// 최초 인벤토리와 UI 동기화
		for (const FInventoryItemEntry& Entry : InventoryComponent->GetInventory().GetItems())
		{
			UpdateInventorySlot(Entry);
		}
	}
	else
	{
		BoundInventoryComponent = nullptr;
		
		if (InventoryUniformGridPanel)
		{
			InventoryUniformGridPanel->ClearChildren();
		}
		
		SlotWidgets.Empty();
	}
	
	UERNEquipmentComponent* EquipmentComponent = GetEquipmentComponent();
	if (EquipmentComponent)
	{
		// 현재 캐릭터의 장착 컴포넌트 설정
		BoundEquipmentComponent = EquipmentComponent;
		
		// 장비 슬롯 갱신 이벤트 구독
		EquipmentComponent->OnEquipmentSlotChanged.AddUniqueDynamic(this, &UERNInventoryWidget::UpdateEquipableSlot);
		EquipmentComponent->OnConsumableSlotChanged.AddUniqueDynamic(this, &UERNInventoryWidget::UpdateConsumableSlot);
		
		// 최초 장비 슬롯과 UI 동기화
		UpdateEquipableSlot(EquipmentComponent->EquipableSlot);
		UpdateConsumableSlot(EquipmentComponent->ConsumableSlot);
	}
	else
	{
		BoundEquipmentComponent = nullptr;
		
		if (EquipableSlotWidget)
		{
			EquipableSlotWidget->ClearItem();
		}
		
		if (ConsumableSlotWidget)
		{
			ConsumableSlotWidget->ClearItem();
		}
	}
}

void UERNInventoryWidget::NativeDestruct()
{
	// 현재 캐릭터 컴포넌트에 이벤트 언바인딩
	UnbindFromCurrentComponent();
	
	// 슬라이드 위젯 확인, 취소 버튼 클릭 이벤트 구독 해제
	if (WBP_SlideWidget)
	{
		WBP_SlideWidget->OnConfirmButtonClicked.RemoveDynamic(this, &UERNInventoryWidget::UpdateSlideWidget);
		WBP_SlideWidget->OnCancelButtonClicked.RemoveDynamic(this, &UERNInventoryWidget::UpdateSlideWidget);
	}
	
	Super::NativeDestruct();
}

void UERNInventoryWidget::UnbindFromCurrentComponent()
{
	if (UERNInventoryComponent* InventoryComponent = BoundInventoryComponent.Get())
	{
		InventoryComponent->OnInventorySlotChanged.RemoveDynamic(this, &UERNInventoryWidget::UpdateInventorySlot);
	}
	
	if (UERNEquipmentComponent* EquipmentComponent = BoundEquipmentComponent.Get())
	{
		EquipmentComponent->OnEquipmentSlotChanged.RemoveDynamic(this, &UERNInventoryWidget::UpdateEquipableSlot);
		EquipmentComponent->OnConsumableSlotChanged.RemoveDynamic(this, &UERNInventoryWidget::UpdateConsumableSlot);
	}
	
	BoundInventoryComponent = nullptr;
	BoundEquipmentComponent = nullptr;
}

FReply UERNInventoryWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	UERNInventoryComponent* InventoryComponent = GetInventoryComponent();
	if (!InventoryComponent)
	{
		return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
	}
	UERNEquipmentComponent* EquipmentComponent = GetEquipmentComponent();
	if (!EquipmentComponent)
	{
		return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
	}
	
	// 키보드 I를 누를 경우 또는 활성화 슬롯이 없는 상태에서 Esc키 누를 경우 (=인벤토리 UI 숨기기)
	if (InKeyEvent.GetKey() == EKeys::I || (FocusSlotIndex == -1 && InKeyEvent.GetKey() == EKeys::Escape))
	{
		if (AERNPlayerController* PC = GetOwningPlayer<AERNPlayerController>())
		{
			// 인벤토리 위젯 숨기기
			PC->ToggleInventory();
		
			return FReply::Handled();
		}
	}
	
	// 활성화된 슬롯 인덱스가 있을 경우
	if (FocusSlotIndex != -1)
	{
		// 키보드 Esc를 누를 경우 (=활성화 슬롯 초기화)
		if (InKeyEvent.GetKey() == EKeys::Escape)
		{
			UpdateFocusSlotIndex(-1);
			
			return FReply::Handled();
		}
		// 키보드 G를 누를 경우 (=아이템 버리기)
		if (InKeyEvent.GetKey() == EKeys::G)
		{
			if (SlotWidgets.IsValidIndex(FocusSlotIndex))
			{
				const int32 Quantity = InventoryComponent->GetItemQuantity(FocusSlotIndex);
				if (Quantity <= 1)
				{
					InventoryComponent->Server_RemoveItem(FocusSlotIndex, Quantity);
				}
				else
				{
					WBP_SlideWidget->InitSlideWidget(TEXT("버릴 수량"), Quantity);
					WBP_SlideWidget->SetVisibility(ESlateVisibility::Visible);
				}
			}
			else
			{
				const int32 Quantity = EquipmentComponent->GetCurrentConsumableQuantity();
				if (Quantity <= 1)
				{
					EquipmentComponent->Server_UnequipItem(Quantity);
				}
				else
				{
					WBP_SlideWidget->InitSlideWidget(TEXT("버릴 수량"), Quantity);
					WBP_SlideWidget->SetVisibility(ESlateVisibility::Visible);
				}
			}
			
			return FReply::Handled();
		}
		// 키보드 E키를 누를 경우 (=아이템 장착)
		if (InKeyEvent.GetKey() == EKeys::E)
		{
			EquipmentComponent->Server_EquipItem(FocusSlotIndex);
			
			return FReply::Handled();
		}
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

FReply UERNInventoryWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && !WBP_SlideWidget->IsVisible())
	{
		UpdateFocusSlotIndex(-1);
		
		return FReply::Handled();
	}
	
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UERNInventoryWidget::NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	UERNInventoryComponent* InventoryComponent = GetInventoryComponent();
	if (!InventoryComponent)
	{
		return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
	}
	
	// 활성화된 슬롯 인덱스가 있을 경우
	if (FocusSlotIndex != -1)
	{
		// 인벤토리 네비게이션 (W/Up, A/Left, S/Down, D/Right)
		const int32 NextIndex = GetNavigationTargetSlotIndex(InKeyEvent.GetKey(), InventoryComponent->GetMaxStackSize());
		if (NextIndex != INDEX_NONE)
		{			
			UpdateFocusSlotIndex(NextIndex);
			
			return FReply::Handled();
		}
	}
	
	return Super::NativeOnPreviewKeyDown(InGeometry, InKeyEvent);
}

const int32 UERNInventoryWidget::GetNavigationTargetSlotIndex(const FKey& Key, const int32 MaxSlotSize) const
{
	int32 NextIndex = INDEX_NONE;
	
	// 위
	if (Key == EKeys::Up)
	{
		NextIndex = (FocusSlotIndex - ColumnSize < 0) ? 
		FocusSlotIndex + ColumnSize * (((FocusSlotIndex - ColumnSize) * -1 + MaxSlotSize - 1) / ColumnSize - 1) : 
		FocusSlotIndex - ColumnSize;
		/* 
		 * Column = FocusSlotIndex % ColumnSize;
		 * Column + ((MaxSlotSize - 1 - Column) / ColumnSize) * ColumnSize;
		 */
	}
	// 아래
	else if (Key == EKeys::Down)
	{
		NextIndex = FocusSlotIndex + ColumnSize >= MaxSlotSize ?
		(FocusSlotIndex + ColumnSize) % ColumnSize : 
		FocusSlotIndex + ColumnSize;
	}
	// 왼쪽
	else if (Key == EKeys::Left)
	{
		NextIndex = (FocusSlotIndex - 1 < 0) ? MaxSlotSize - 1 : FocusSlotIndex - 1;
	}
	// 오른쪽
	else if (Key == EKeys::Right)
	{
		NextIndex = (FocusSlotIndex + 1) % MaxSlotSize;
	}
	
	return NextIndex;
}

void UERNInventoryWidget::CreateSlot(const int32 MaxSlotSize, const int32 ColumnCount)
{
	InventoryUniformGridPanel->ClearChildren();
	SlotWidgets.SetNum(MaxSlotSize);
	
	for (int32 i = 0; i < MaxSlotSize; ++i)
	{
		SlotWidgets[i] = CreateWidget<UERNInventorySlotWidget>(this, SlotWidgetClass);
		
		const int32 Row = i / ColumnCount;
		const int32 Col = i % ColumnCount;
		
		InventoryUniformGridPanel->AddChildToUniformGrid(SlotWidgets[i], Row, Col);
		
		if (SlotWidgets[i])
		{
			SlotWidgets[i]->SetSlotIndex(i);
			// 인벤토리 슬롯 활성화 이벤트 구독
			SlotWidgets[i]->OnSlotClicked.AddUniqueDynamic(this, &UERNInventoryWidget::UpdateFocusSlotIndex);
		}
	}
	
	// 소모품 슬롯 인덱스 설정
	ConsumableSlotWidget->SetSlotIndex(MaxSlotSize);
	ConsumableSlotWidget->OnSlotClicked.AddUniqueDynamic(this, &UERNInventoryWidget::UpdateFocusSlotIndex);
}

void UERNInventoryWidget::UpdateInventorySlot(const FInventoryItemEntry& Entry)
{
	// 슬롯 위젯 유효성 확인
	if (!SlotWidgets.IsValidIndex(Entry.GetSlotIndex()))
	{
		return;
	}
	
	// 수량이 0인 경우 슬롯 위젯 초기화
	if (Entry.GetQuantity() <= 0)
	{
		SlotWidgets[Entry.GetSlotIndex()]->ClearItem();
		return;
	}
	
	if (UItemManagerSubsystem* ItemManager = GetGameInstance()->GetSubsystem<UItemManagerSubsystem>())
	{
		// TODO: 비동기 로드 변경
		// ItemManager에서 동기 로드로 UI 리소스 로드
		if (const UItemDataAssetBase* ItemData = ItemManager->LoadItemDataAssetSync(Entry.GetItemID(), EItemAssetLoadFlags::UI))
		{
			// 슬롯 위젯 갱신
			SlotWidgets[Entry.GetSlotIndex()]->SetItem(ItemData->Icon.Get(), Entry.GetQuantity());
		}
	}
}

void UERNInventoryWidget::UpdateEquipableSlot(const FInventoryItemEntry& Entry)
{
	if (Entry.GetItemID().IsNone() || Entry.GetQuantity() <= 0)
	{
		EquipableSlotWidget->ClearItem();
		return;
	}
	
	if (UItemManagerSubsystem* ItemManager = GetGameInstance()->GetSubsystem<UItemManagerSubsystem>())
	{
		// TODO: 비동기 로드 변경
		// ItemManager에서 동기 로드로 UI 리소스 로드
		if (const UItemDataAssetBase* ItemData = ItemManager->LoadItemDataAssetSync(Entry.GetItemID(), EItemAssetLoadFlags::UI))
		{
			// 슬롯 위젯 갱신
			EquipableSlotWidget->SetItem(ItemData->Icon.Get(), Entry.GetQuantity());
		}
	}
}

void UERNInventoryWidget::UpdateConsumableSlot(const FInventoryItemEntry& Entry)
{
	if (Entry.GetItemID().IsNone() || Entry.GetQuantity() <= 0)
	{
		ConsumableSlotWidget->ClearItem();
		return;
	}
	
	if (UItemManagerSubsystem* ItemManager = GetGameInstance()->GetSubsystem<UItemManagerSubsystem>())
	{
		if (Entry.GetItemID() == NAME_None)
		{
			ConsumableSlotWidget->ClearItem();
			return;
		}
		
		// TODO: 비동기 로드 변경
		// ItemManager에서 동기 로드로 UI 리소스 로드
		if (const UItemDataAssetBase* ItemData = ItemManager->LoadItemDataAssetSync(Entry.GetItemID(), EItemAssetLoadFlags::UI))
		{
			// 슬롯 위젯 갱신
			ConsumableSlotWidget->SetItem(ItemData->Icon.Get(), Entry.GetQuantity());
		}
	}
}

void UERNInventoryWidget::UpdateFocusSlotIndex(const int32 NewIndex)
{
	if (WBP_SlideWidget->IsVisible())
	{
		return;
	}
		
	// 매개변수 유효성 검사
	if (NewIndex < -1 || NewIndex > SlotWidgets.Num())
	{
		return;
	}
	
	// 활성화된 슬롯이 있을 경우
	if (FocusSlotIndex == SlotWidgets.Num())
	{
		ConsumableSlotWidget->SetInventorySlotImage(BasicSlotImage.Get());
	}
	else if (FocusSlotIndex != -1)
	{
		// 비활성화 UI로 변경
		SlotWidgets[FocusSlotIndex]->SetInventorySlotImage(BasicSlotImage.Get());
	}
	
	// 슬롯 초기화(-1)가 아니라면
	if (NewIndex == SlotWidgets.Num())
	{
		ConsumableSlotWidget->SetInventorySlotImage(FocusSlotImage.Get());
	}
	else if (NewIndex != -1)
	{
		// 활성화 UI로 변경
		SlotWidgets[NewIndex]->SetInventorySlotImage(FocusSlotImage.Get());
	}
	
	// 활성화 슬롯 인덱스 갱신
	FocusSlotIndex = NewIndex;
}

void UERNInventoryWidget::UpdateSlideWidget(const int32 NewQuantity)
{
	if (SlotWidgets.IsValidIndex(FocusSlotIndex))
	{
		if (UERNInventoryComponent* InventoryComponent = GetInventoryComponent())
		{
			InventoryComponent->Server_RemoveItem(FocusSlotIndex, NewQuantity);
		}
	}
	else
	{
		if (UERNEquipmentComponent* EquipmentComponent = GetEquipmentComponent())
		{
			EquipmentComponent->Server_UnequipItem(NewQuantity);
		}
	}
	WBP_SlideWidget->SetVisibility(ESlateVisibility::Hidden);
}

UERNInventoryComponent* UERNInventoryWidget::GetInventoryComponent() const
{
	if (const AERNPlayerController* PC = GetOwningPlayer<AERNPlayerController>())
	{
		if (const AProjectERNCharacter* PlayerCharacter = Cast<AProjectERNCharacter>(PC->GetCharacter()))
		{
			return PlayerCharacter->GetInventoryComponent();
		}
	}
	
	return nullptr;
}

UERNEquipmentComponent* UERNInventoryWidget::GetEquipmentComponent() const
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
