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
#include "TimerManager.h"
#include "Engine/World.h"

UERNInventoryWidget::UERNInventoryWidget(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	SetIsFocusable(true);
}

void UERNInventoryWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	// 처음 위젯이 생성될 때 숨김 처리
	SetVisibility(ESlateVisibility::Hidden);
	
	// 현재 캐릭터 컴포넌트에 이벤트 바인딩
	RefreshFromCurrentCharacter();
	
	// 슬라이드 위젯 확인, 취소 버튼 이벤트 바인딩
	if (WBP_SlideWidget)
	{
		WBP_SlideWidget->OnConfirmButtonClicked.AddUniqueDynamic(this, &UERNInventoryWidget::UpdateSlideWidget);
		WBP_SlideWidget->OnCancelButtonClicked.AddUniqueDynamic(this, &UERNInventoryWidget::UpdateSlideWidget);
	}
}

void UERNInventoryWidget::RefreshFromCurrentCharacter()
{
	// 현재 캐릭터 컴포넌트에 이벤트 언바인딩
	UnbindFromCurrentComponent();
	
	// 활성화된 슬롯 인덱스 초기화
	InitFocusSlotIndex();

	// 인벤토리 컴포넌트 이벤트 바인딩
	if (UERNInventoryComponent* InventoryComponent = GetInventoryComponent())
	{
		// 현재 바인딩된 인벤토리 컴포넌트 포인터를 현재 캐릭터의 인벤토리 컴포넌트로 설정
		BoundInventoryComponent = InventoryComponent;
		
		// 슬롯 생성
		CreateSlot(InventoryComponent->GetMaxSlotSize(), ColumnSize);
		
		// 인벤토리 갱신 이벤트 바인딩
		InventoryComponent->OnInventorySlotChanged.AddUniqueDynamic(this, &UERNInventoryWidget::UpdateInventorySlot);
		
		// 최초 바인딩 후 인벤토리 컴포넌트와 UI 동기화
		for (const FInventoryItemEntry& Entry : InventoryComponent->GetInventory().GetItems())
		{
			UpdateInventorySlot(Entry);
		}
	}
	else
	{
		// 현재 바인딩된 인벤토리 컴포넌트 포인터 초기화
		BoundInventoryComponent = nullptr;
		
		// 인벤토리 그리드 패널 초기화
		InventoryUniformGridPanel->ClearChildren();
		
		// 인벤토리 슬롯 배열 초기화
		SlotWidgets.Empty();
	}

	// 장착 컴포넌트 이벤트 바인딩
	if (UERNEquipmentComponent* EquipmentComponent = GetEquipmentComponent())
	{
		// 현재 바인딩된 장착 컴포넌트 포인터를 현재 캐릭터의 장착 컴포넌트로 설정
		BoundEquipmentComponent = EquipmentComponent;
		
		// 장비 슬롯 갱신 이벤트 바인딩
		EquipmentComponent->OnEquipmentSlotChanged.AddUniqueDynamic(this, &UERNInventoryWidget::UpdateEquipableSlot);
		EquipmentComponent->OnConsumableSlotChanged.AddUniqueDynamic(this, &UERNInventoryWidget::UpdateConsumableSlot);
		
		// 최초 바인딩 후 장비 컴포넌트와 UI 동기화
		UpdateEquipableSlot(EquipmentComponent->EquipableSlot);
		UpdateConsumableSlot(EquipmentComponent->ConsumableSlot);
	}
	else
	{
		// 현재 바인딩된 장비 컴포넌트 포인터 초기화
		BoundEquipmentComponent = nullptr;
		
		// 무기, 소모품 슬롯 초기화
		EquipableSlotWidget->ClearItem();
		ConsumableSlotWidget->ClearItem();
	}
}

void UERNInventoryWidget::CreateSlot(const int32 MaxSlotSize, const int32 ColumnCount)
{
	// 인벤토리 그리드 패널 초기화
	InventoryUniformGridPanel->ClearChildren();
	// 인벤토리 슬롯 배열 크기 설정
	SlotWidgets.SetNum(MaxSlotSize);
	
	// 인벤토리 슬롯 생성
	for (int32 i = 0; i < MaxSlotSize; ++i)
	{
		SlotWidgets[i] = CreateWidget<UERNInventorySlotWidget>(this, SlotWidgetClass);
		
		const int32 Row = i / ColumnCount;
		const int32 Col = i % ColumnCount;
		
		InventoryUniformGridPanel->AddChildToUniformGrid(SlotWidgets[i], Row, Col);
		
		if (SlotWidgets[i])
		{
			// 인벤토리 슬롯 인덱스 설정
			SlotWidgets[i]->SetSlotIndex(i);
			// 인벤토리 슬롯 활성화 이벤트 바인딩
			SlotWidgets[i]->OnSlotClicked.AddUniqueDynamic(this, &UERNInventoryWidget::UpdateFocusSlotIndex);
			SlotWidgets[i]->OnSlotHovered.AddUniqueDynamic(this, &UERNInventoryWidget::OnSlotHoveredCallback);
			SlotWidgets[i]->OnSlotUnhovered.AddUniqueDynamic(this, &UERNInventoryWidget::OnSlotUnhoveredCallback);
			SlotWidgets[i]->OnSlotDoubleClicked.AddUniqueDynamic(this, &UERNInventoryWidget::OnSlotDoubleClickedCallback);
		}
	}
	
	// 소모품 슬롯 인덱스 설정
	ConsumableSlotWidget->SetSlotIndex(MaxSlotSize);
	// 소모품 슬롯 활성화 이벤트 바인딩
	ConsumableSlotWidget->OnSlotClicked.AddUniqueDynamic(this, &UERNInventoryWidget::UpdateFocusSlotIndex);
	ConsumableSlotWidget->OnSlotHovered.AddUniqueDynamic(this, &UERNInventoryWidget::OnSlotHoveredCallback);
	ConsumableSlotWidget->OnSlotUnhovered.AddUniqueDynamic(this, &UERNInventoryWidget::OnSlotUnhoveredCallback);
	ConsumableSlotWidget->OnSlotDoubleClicked.AddUniqueDynamic(this, &UERNInventoryWidget::OnSlotDoubleClickedCallback);
	// 소모품 슬롯 이미지 설정
	ConsumableSlotWidget->SetInventorySlotImage(BasicConsumableSlotImage.Get());
	
	// 장비 슬롯 인덱스 설정 (소모품 슬롯 다음 인덱스)
	EquipableSlotWidget->SetSlotIndex(MaxSlotSize + 1);
	// 장비 슬롯 활성화 이벤트 바인딩
	EquipableSlotWidget->OnSlotClicked.AddUniqueDynamic(this, &UERNInventoryWidget::UpdateFocusSlotIndex);
	EquipableSlotWidget->OnSlotHovered.AddUniqueDynamic(this, &UERNInventoryWidget::OnSlotHoveredCallback);
	EquipableSlotWidget->OnSlotUnhovered.AddUniqueDynamic(this, &UERNInventoryWidget::OnSlotUnhoveredCallback);
	EquipableSlotWidget->OnSlotDoubleClicked.AddUniqueDynamic(this, &UERNInventoryWidget::OnSlotDoubleClickedCallback);
	// 장비 슬롯 이미지 설정 (장비 전용 이미지가 없으므로 기본 슬롯 이미지 사용)
	EquipableSlotWidget->SetInventorySlotImage(BasicSlotImage.Get());
}

void UERNInventoryWidget::NativeDestruct()
{
	// 현재 캐릭터 컴포넌트에 이벤트 언바인딩
	UnbindFromCurrentComponent();
	
	// 슬라이드 위젯 확인, 취소 버튼 이벤트 언바인딩
	if (WBP_SlideWidget)
	{
		WBP_SlideWidget->OnConfirmButtonClicked.RemoveDynamic(this, &UERNInventoryWidget::UpdateSlideWidget);
		WBP_SlideWidget->OnCancelButtonClicked.RemoveDynamic(this, &UERNInventoryWidget::UpdateSlideWidget);
	}
	
	Super::NativeDestruct();
}

void UERNInventoryWidget::UnbindFromCurrentComponent()
{
	// 인벤토리 컴포넌트 이벤트 바인딩 해제
	if (UERNInventoryComponent* InventoryComponent = BoundInventoryComponent.Get())
	{
		InventoryComponent->OnInventorySlotChanged.RemoveDynamic(this, &UERNInventoryWidget::UpdateInventorySlot);
	}
	
	// 장착 컴포넌트 이벤트 바인딩 해제
	if (UERNEquipmentComponent* EquipmentComponent = BoundEquipmentComponent.Get())
	{
		EquipmentComponent->OnEquipmentSlotChanged.RemoveDynamic(this, &UERNInventoryWidget::UpdateEquipableSlot);
		EquipmentComponent->OnConsumableSlotChanged.RemoveDynamic(this, &UERNInventoryWidget::UpdateConsumableSlot);
	}
	
	// 현재 바인딩된 인벤토리, 장착 컴포넌트 포인터 초기화
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
	
	// 키보드 I를 누를 경우 (=인벤토리 UI 숨기기)
	if (InKeyEvent.GetKey() == EKeys::I)
	{
		// UI 닫기 처리 BP 함수 호출 (부모에서 선언된 함수)
		BP_PlayCloseAnimation();
		
		return FReply::Handled();
	}
	
	// 활성화된 슬롯 인덱스가 있을 경우 (Hover가 최우선, 없으면 고정 Focus)
	const int32 ActiveIndex = (HoveredSlotIndex != -1) ? HoveredSlotIndex : FocusSlotIndex;
	if (ActiveIndex != -1)
	{
		// 키보드 Esc를 누를 경우 (=활성화 슬롯 초기화)
		if (InKeyEvent.GetKey() == EKeys::Escape)
		{
			InitFocusSlotIndex();
			
			return FReply::Handled();
		}
		// 키보드 G를 누를 경우 (=아이템 버리기)
		if (InKeyEvent.GetKey() == EKeys::G)
		{
			if (SlotWidgets.IsValidIndex(ActiveIndex))
			{
				const int32 Quantity = InventoryComponent->GetItemQuantity(ActiveIndex);
				if (Quantity <= 1)
				{
					InventoryComponent->Server_RemoveItem(ActiveIndex, Quantity);
				}
				else
				{
					WBP_SlideWidget->InitSlideWidget(TEXT("버릴 수량"), Quantity);
					WBP_SlideWidget->SetVisibility(ESlateVisibility::Visible);
				}
			}
			else if (ActiveIndex == InventoryComponent->GetMaxSlotSize()) // 소모품 슬롯
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
			else if (ActiveIndex == InventoryComponent->GetMaxSlotSize() + 1) // 장비 슬롯
			{
				// TODO: 백엔드 지원 시 주석 해제 및 구현
				// EquipmentComponent->Server_DropWeapon();
				UE_LOG(LogTemp, Warning, TEXT("Drop Weapon from Equipable Slot - Needs Backend Support"));
			}
			
			return FReply::Handled();
		}
		// 키보드 E키를 누를 경우 (=아이템 장착)
		if (InKeyEvent.GetKey() == EKeys::E)
		{
			if (SlotWidgets.IsValidIndex(ActiveIndex))
			{
				EquipmentComponent->Server_EquipItem(ActiveIndex);
				InitFocusSlotIndex(); // 장착 완료 후 포커스 자동 해제
			}
			else if (ActiveIndex == InventoryComponent->GetMaxSlotSize() + 1) // 장비 슬롯 (장착 해제)
			{
				// TODO: 백엔드 지원 시 주석 해제 및 구현
				// EquipmentComponent->Server_UnequipWeaponToInventory();
				UE_LOG(LogTemp, Warning, TEXT("Unequip Weapon to Inventory - Needs Backend Support"));
			}
			
			return FReply::Handled();
		}
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

FReply UERNInventoryWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// 슬롯이 아닌 곳을 클릭하면 활성화 슬롯 초기화
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton ||
		InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		if (!WBP_SlideWidget->IsVisible())
		{
			InitFocusSlotIndex();
			return FReply::Handled();
		}
	}
	
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UERNInventoryWidget::NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const UERNInventoryComponent* InventoryComponent = GetInventoryComponent();
	if (!InventoryComponent)
	{
		return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
	}
	
	// 활성화된 슬롯 인덱스가 있을 경우
	const int32 ActiveIndex = (HoveredSlotIndex != -1) ? HoveredSlotIndex : FocusSlotIndex;
	if (ActiveIndex != -1)
	{
		// 인벤토리 네비게이션 (Up, Left, Down, Right)
		const int32 NextIndex = GetNavigationTargetSlotIndex(InKeyEvent.GetKey(), InventoryComponent->GetMaxSlotSize(), ActiveIndex);
		if (NextIndex != INDEX_NONE)
		{			
			UpdateFocusSlotIndex(NextIndex);
			
			return FReply::Handled();
		}
	}
	
	return Super::NativeOnPreviewKeyDown(InGeometry, InKeyEvent);
}

const int32 UERNInventoryWidget::GetNavigationTargetSlotIndex(const FKey& Key, const int32 MaxSlotSize, const int32 CurrentIndex) const
{
	int32 NextIndex = INDEX_NONE;
	
	// 위
	if (Key == EKeys::Up)
	{
		NextIndex = (CurrentIndex - ColumnSize < 0) ? 
		CurrentIndex + ColumnSize * (((CurrentIndex - ColumnSize) * -1 + MaxSlotSize - 1) / ColumnSize - 1) : 
		CurrentIndex - ColumnSize;
	}
	// 아래
	else if (Key == EKeys::Down)
	{
		NextIndex = CurrentIndex + ColumnSize >= MaxSlotSize ?
		(CurrentIndex + ColumnSize) % ColumnSize : 
		CurrentIndex + ColumnSize;
	}
	// 왼쪽
	else if (Key == EKeys::Left)
	{
		NextIndex = (CurrentIndex - 1 < 0) ? MaxSlotSize - 1 : CurrentIndex - 1;
	}
	// 오른쪽
	else if (Key == EKeys::Right)
	{
		NextIndex = (CurrentIndex + 1) % MaxSlotSize;
	}
	
	return NextIndex;
}

void UERNInventoryWidget::UpdateInventorySlot(const FInventoryItemEntry& Entry)
{	
	// 슬롯 위젯 유효성 확인
	if (!SlotWidgets.IsValidIndex(Entry.GetSlotIndex()))
	{
		return;
	}
	
	// 아이템 키값이 유효하지 않거나 수량이 0인 경우 슬롯 위젯 초기화
	if (Entry.GetItemID().IsNone() || Entry.GetItemID() == NAME_None || Entry.GetQuantity() <= 0)
	{
		UERNInventorySlotWidget* SlotWidget = SlotWidgets[Entry.GetSlotIndex()];

		SlotWidget->ClearItem();

		if (FocusSlotIndex == Entry.GetSlotIndex())
		{
			SlotWidget->SetInventorySlotImage(FocusSlotImage.Get());
		}
		else
		{
			SlotWidget->SetInventorySlotImage(BasicSlotImage.Get());
		}
		SlotWidget->SetInventorySlotTint(FColor::White);

		return;
	}
	
	// ItemManager에서 UI 리소스 비로드
	if (UItemManagerSubsystem* ItemManager = GetGameInstance()->GetSubsystem<UItemManagerSubsystem>())
	{
		TWeakObjectPtr<UERNInventoryWidget> WeakThis(this);
		const FName ItemID = Entry.GetItemID();
		const int32 SlotIndex = Entry.GetSlotIndex();
		const int32 Quantity = Entry.GetQuantity();
		EItemGrade Grade = ItemManager->FindItemRow(Entry.GetItemID())->Grade;
		
		ItemManager->PreloadItemDataAssetAsync(Entry.GetItemID(), EItemAssetLoadFlags::UI, 
			FOnItemDataAssetLoaded::CreateLambda([WeakThis, ItemID, SlotIndex, Quantity, Grade](const UItemDataAssetBase* ItemData)
			{
				if (!WeakThis.IsValid() || !ItemData)
				{
					return;
				}
				
				if (!WeakThis->SlotWidgets.IsValidIndex(SlotIndex))
				{
					return;
				}
				
				const UERNInventoryComponent* InventoryComponent = WeakThis->GetInventoryComponent();
				if (!InventoryComponent || !InventoryComponent->GetInventory().GetItems().IsValidIndex(SlotIndex))
				{
					return;
				}

				const FInventoryItemEntry& CurrentEntry = InventoryComponent->GetInventory().GetItems()[SlotIndex];

				if (CurrentEntry.GetItemID() != ItemID || CurrentEntry.GetQuantity() != Quantity)
				{
					return;
				}
				
				WeakThis->SlotWidgets[SlotIndex]->SetItem(ItemData->Icon.Get(), Quantity, WeakThis->ItemGradeByColor(Grade));
				
				WeakThis->UpdateFocusSlotIndex(WeakThis->FocusSlotIndex);
			}
		));
	}
}

void UERNInventoryWidget::UpdateEquipableSlot(const FInventoryItemEntry& Entry)
{
	// 아이템 키값이 유효하지 않거나 수량이 0인 경우 슬롯 위젯 초기화
	if (Entry.GetItemID().IsNone() || Entry.GetItemID() == NAME_None || Entry.GetQuantity() <= 0)
	{
		EquipableSlotWidget->ClearItem();
		return;
	}
	
	// ItemManager에서 UI 리소스 비로드
	if (UItemManagerSubsystem* ItemManager = GetGameInstance()->GetSubsystem<UItemManagerSubsystem>())
	{
		TWeakObjectPtr<UERNInventoryWidget> WeakThis(this);
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
				
				WeakThis->UpdateFocusSlotIndex(WeakThis->FocusSlotIndex);
			}
		));
	}
}

void UERNInventoryWidget::UpdateConsumableSlot(const FInventoryItemEntry& Entry)
{
	// 아이템 키값이 유효하지 않거나 수량이 0인 경우 슬롯 위젯 초기화
	if (Entry.GetItemID().IsNone() || Entry.GetItemID() == NAME_None || Entry.GetQuantity() <= 0)
	{
		ConsumableSlotWidget->ClearItem();

		if (FocusSlotIndex == SlotWidgets.Num())
		{
			ConsumableSlotWidget->SetInventorySlotImage(FocusConsumableSlotImage.Get());
		}
		else
		{
			ConsumableSlotWidget->SetInventorySlotImage(BasicConsumableSlotImage.Get());
		}
		ConsumableSlotWidget->SetInventorySlotTint(FColor::White);
		
		return;
	}
	
	// ItemManager에서 UI 리소스 비로드
	if (UItemManagerSubsystem* ItemManager = GetGameInstance()->GetSubsystem<UItemManagerSubsystem>())
	{
		TWeakObjectPtr<UERNInventoryWidget> WeakThis(this);
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
				
				WeakThis->UpdateFocusSlotIndex(WeakThis->FocusSlotIndex);
			}
		));
	}
}

void UERNInventoryWidget::UpdateFocusSlotIndex(const int32 NewIndex)
{
	// 슬라이드 위젯으로 갯수 조정 중일 때는 활성화 슬롯 변경 불가
	if (WBP_SlideWidget->IsVisible())
	{
		return;
	}
		
	// 매개변수 유효성 검사
	if (NewIndex < -1 || NewIndex > SlotWidgets.Num())
	{
		return;
	}
	
	// 활성화 슬롯 인덱스 갱신 (고정 상태 업데이트)
	FocusSlotIndex = NewIndex;
	UpdateVisuals();
}

void UERNInventoryWidget::UpdateVisuals()
{
	// 1. 모든 슬롯 비활성화 UI로 초기화
	for (int32 i = 0; i < SlotWidgets.Num(); ++i)
	{
		if (SlotWidgets[i])
		{
			SlotWidgets[i]->SetInventorySlotImage(BasicSlotImage.Get());
			SlotWidgets[i]->InitInventorySlotTint();
		}
	}
	if (ConsumableSlotWidget)
	{
		if (UERNEquipmentComponent* EquipmentComp = GetEquipmentComponent())
		{
			if (EquipmentComp->ConsumableSlot.GetQuantity() > 0)
			{
				ConsumableSlotWidget->SetInventorySlotImage(BasicSlotImage.Get());
			}
			else
			{
				ConsumableSlotWidget->SetInventorySlotImage(BasicConsumableSlotImage.Get());
			}
		}
		else
		{
			ConsumableSlotWidget->SetInventorySlotImage(BasicConsumableSlotImage.Get());
		}
		ConsumableSlotWidget->InitInventorySlotTint();
	}
	if (EquipableSlotWidget)
	{
		EquipableSlotWidget->SetInventorySlotImage(BasicSlotImage.Get());
		EquipableSlotWidget->InitInventorySlotTint();
	}
	
	// 2. 표시할(하이라이트 및 툴팁) 단일 인덱스 결정 (Hover가 최우선)
	int32 DisplayIndex = -1;
	if (HoveredSlotIndex != -1)
	{
		DisplayIndex = HoveredSlotIndex;
	}
	else if (FocusSlotIndex != -1)
	{
		DisplayIndex = FocusSlotIndex;
	}
	
	// 3. 결정된 슬롯 1개만 하이라이트 적용
	if (DisplayIndex != -1)
	{
		if (DisplayIndex == SlotWidgets.Num() + 1) // 장비 슬롯
		{
			EquipableSlotWidget->SetInventorySlotImage(FocusSlotImage.Get());
			EquipableSlotWidget->SetInventorySlotTint(FColor::White);
		}
		else if (DisplayIndex == SlotWidgets.Num()) // 소모품 슬롯
		{
			if (UERNEquipmentComponent* EquipmentComp = GetEquipmentComponent())
			{
				if (EquipmentComp->ConsumableSlot.GetQuantity() > 0)
				{
					ConsumableSlotWidget->SetInventorySlotImage(FocusSlotImage.Get());
				}
				else
				{
					ConsumableSlotWidget->SetInventorySlotImage(FocusConsumableSlotImage.Get());
				}
			}
			else
			{
				ConsumableSlotWidget->SetInventorySlotImage(FocusConsumableSlotImage.Get());
			}
			ConsumableSlotWidget->SetInventorySlotTint(FColor::White);
		}
		else if (SlotWidgets.IsValidIndex(DisplayIndex)) // 일반 인벤토리 슬롯
		{
			SlotWidgets[DisplayIndex]->SetInventorySlotImage(FocusSlotImage.Get());
			SlotWidgets[DisplayIndex]->SetInventorySlotTint(FColor::White);
		}
	}
	
	// 4. 툴팁 갱신 (DisplayIndex 기준)
	if (WBP_ItemToolTip)
	{
		if (DisplayIndex == -1)
		{
			WBP_ItemToolTip->SetVisibility(ESlateVisibility::Collapsed);
		}
		else
		{
			FName ItemID = NAME_None;
			if (DisplayIndex == SlotWidgets.Num() + 1) // 장비 슬롯
			{
				if (UERNEquipmentComponent* EquipComp = GetEquipmentComponent())
				{
					ItemID = EquipComp->EquipableSlot.GetItemID();
				}
			}
			else if (DisplayIndex == SlotWidgets.Num()) // 소모품 슬롯
			{
				if (UERNEquipmentComponent* EquipComp = GetEquipmentComponent())
				{
					ItemID = EquipComp->ConsumableSlot.GetItemID();
				}
			}
			else if (SlotWidgets.IsValidIndex(DisplayIndex)) // 일반 인벤토리 슬롯
			{
				if (UERNInventoryComponent* InvComp = GetInventoryComponent())
				{
					if (InvComp->GetInventory().GetItems().IsValidIndex(DisplayIndex))
					{
						ItemID = InvComp->GetInventory().GetItems()[DisplayIndex].GetItemID();
					}
				}
			}
			
			if (!ItemID.IsNone() && ItemID != NAME_None)
			{
				WBP_ItemToolTip->UpdateTooltip(ItemID, 0); // 가격 0으로 표시
				WBP_ItemToolTip->SetVisibility(ESlateVisibility::HitTestInvisible);
			}
			else
			{
				WBP_ItemToolTip->SetVisibility(ESlateVisibility::Collapsed);
			}
		}
	}
}

void UERNInventoryWidget::OnSlotHoveredCallback(const int32 Index)
{
	if (WBP_SlideWidget->IsVisible()) return;
	
	// 진행 중인 Unhover 지연 타이머가 있다면 취소 (깜빡임 방지)
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(UnhoverTimerHandle);
	}
	
	HoveredSlotIndex = Index;
	UpdateVisuals();
}

void UERNInventoryWidget::OnSlotUnhoveredCallback(const int32 Index)
{
	if (HoveredSlotIndex == Index)
	{
		HoveredSlotIndex = -1;
		
		// 슬롯 사이의 패딩(빈 공간)을 지날 때 툴팁과 하이라이트가 고정 슬롯으로 
		// 순간적으로 튀는 깜빡임(Flicker)을 방지하기 위해 0.05초 지연 복귀
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(
				UnhoverTimerHandle, 
				this, 
				&UERNInventoryWidget::ProcessUnhoverFallback, 
				0.05f, 
				false
			);
		}
		else
		{
			UpdateVisuals();
		}
	}
}

void UERNInventoryWidget::ProcessUnhoverFallback()
{
	// 0.05초가 지났는데도 여전히 Hover된 슬롯이 없다면 고정 슬롯으로 복귀 처리
	if (HoveredSlotIndex == -1)
	{
		UpdateVisuals();
	}
}

void UERNInventoryWidget::OnSlotDoubleClickedCallback(const int32 Index)
{
	if (UERNEquipmentComponent* EquipmentComponent = GetEquipmentComponent())
	{
		if (SlotWidgets.IsValidIndex(Index)) // 인벤토리 -> 장착
		{
			EquipmentComponent->Server_EquipItem(Index);
			InitFocusSlotIndex(); // 장착 완료 후 포커스 자동 해제
		}
		else if (Index == SlotWidgets.Num() + 1) // 장비 슬롯 -> 해제
		{
			// TODO: 백엔드 지원 시 주석 해제 및 구현
			// EquipmentComponent->Server_UnequipWeaponToInventory();
			UE_LOG(LogTemp, Warning, TEXT("Unequip Weapon to Inventory on Double Click - Needs Backend Support"));
		}
	}
}

void UERNInventoryWidget::UpdateSlideWidget(const int32 NewQuantity)
{
	// 인벤토리 슬롯 위젯에 유효한 인덱스라면
	if (SlotWidgets.IsValidIndex(FocusSlotIndex))
	{
		if (UERNInventoryComponent* InventoryComponent = GetInventoryComponent())
		{
			// 인벤토리 컴포넌트에 제거 요청
			InventoryComponent->Server_RemoveItem(FocusSlotIndex, NewQuantity);
		}
	}
	else if (FocusSlotIndex == SlotWidgets.Num()) // 소모품 슬롯
	{
		if (UERNEquipmentComponent* EquipmentComponent = GetEquipmentComponent())
		{
			// 장비 컴포넌트에 아이템 제거 요청 (소모품)
			EquipmentComponent->Server_UnequipItem(NewQuantity);
		}
	}
	
	// 슬라이드 위젯 숨기기
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

void UERNInventoryWidget::InitFocusSlotIndex()
{
	HoveredSlotIndex = -1;
	UpdateFocusSlotIndex(-1);
}

void UERNInventoryWidget::PlayOpenAnimation()
{
	PlayAnimation(FadeIn);
	InitFocusSlotIndex();
}

FColor UERNInventoryWidget::ItemGradeByColor(EItemGrade Grade)
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
