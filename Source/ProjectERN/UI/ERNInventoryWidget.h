// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ERNInteractableWidget.h"
#include "ERNInventorySlotWidget.h"
#include "Inventory/Data/ERNInventoryList.h"
#include "Inventory/Item/Data/ERNItemEnums.h"
#include "ERNItemToolTipWidget.h"
#include "Engine/TimerHandle.h"
#include "ERNInventoryWidget.generated.h"

class UERNEquipmentComponent;
class UERNSlideWidget;
class UUniformGridPanel;
class UERNInventorySlotWidget;

/**
 * 
 */
UCLASS()
class PROJECTERN_API UERNInventoryWidget : public UERNInteractableWidget
{
	GENERATED_BODY()
	
public:
	UERNInventoryWidget(const FObjectInitializer& ObjectInitializer);
	
	// 현재 캐릭터 컴포넌트에 이벤트 바인딩
	void RefreshFromCurrentCharacter();
	
	// 열기 애니메이션 재생
	void PlayOpenAnimation();
	
protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	
private:
	// 인벤토리 컴포넌트 가져오기
	UERNInventoryComponent* GetInventoryComponent() const;
	
	// 장착 컴포넌트 가져오기
	UERNEquipmentComponent* GetEquipmentComponent() const;
	
	// 활성화된 슬롯 인덱스 초기화
	void InitFocusSlotIndex();
	
	// 슬롯 생성
	void CreateSlot(const int32 MaxSlotSize, const int32 ColumnCount);
	
	// 현재 캐릭터 컴포넌트에 이벤트 언바인딩
	void UnbindFromCurrentComponent();
	
	// 인벤토리	// 네비게이션 적용될 슬롯 인덱스 반환 함수
	const int32 GetNavigationTargetSlotIndex(const FKey& Key, const int32 MaxSlotSize, const int32 CurrentIndex) const;
	
	// 등급에 따른 색
	FColor ItemGradeByColor(EItemGrade Grade = EItemGrade::None);
	
	// 인벤토리 슬롯 갱신 이벤트 핸들러
	UFUNCTION()
	void UpdateInventorySlot(const FInventoryItemEntry& Entry);
	
	// 장비 슬롯 갱신 이벤트 핸들러
	UFUNCTION()
	void UpdateEquipableSlot(const FInventoryItemEntry& Entry);
	
	// 소모품 슬롯 갱신 이벤트 핸들러
	UFUNCTION()
	void UpdateConsumableSlot(const FInventoryItemEntry& Entry);
	
	// 슬롯 활성화 이벤트 핸들러
	UFUNCTION()
	void UpdateFocusSlotIndex(const int32 NewIndex);
	
	// 슬라이어 위젯 갱신 이벤트 핸들러
	UFUNCTION()
	void UpdateSlideWidget(const int32 NewQuantity);
	
	// 새로 추가된 호버링, 더블클릭 이벤트 핸들러
	UFUNCTION()
	void OnSlotHoveredCallback(const int32 Index);
	
	UFUNCTION()
	void OnSlotUnhoveredCallback(const int32 Index);
	
	UFUNCTION()
	void OnSlotDoubleClickedCallback(const int32 Index);
	
	// 시각적 상태(하이라이트, 툴팁) 통합 업데이트
	void UpdateVisuals();
	
	// 패딩 사이를 지날 때 깜빡임(Flicker)을 방지하기 위한 지연 복귀 타이머 핸들
	FTimerHandle UnhoverTimerHandle;
	
	UFUNCTION()
	void ProcessUnhoverFallback();
	
private:
	// 장비 슬롯
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UERNInventorySlotWidget> EquipableSlotWidget;
	
	// 소모품 슬롯
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UERNInventorySlotWidget> ConsumableSlotWidget;
	
	// 인벤토리 그리드 패널
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UUniformGridPanel> InventoryUniformGridPanel;
	
	// 슬라이드 위젯
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UERNSlideWidget> WBP_SlideWidget;
	
	// 아이템 툴팁 위젯 (선택적)
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UERNItemToolTipWidget> WBP_ItemToolTip;
	
	// 열기 애니메이션
	UPROPERTY(meta=(BindWidgetAnim), Transient)
	TObjectPtr<UWidgetAnimation> FadeIn;
	
	// 인벤토리 슬롯 위젯 클래스
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Inventory", meta=(AllowPrivateAccess="true"))
	TSubclassOf<UUserWidget> SlotWidgetClass;
	
	// 인벤토리 슬롯 이미지
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Inventory", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UTexture2D> BasicSlotImage = nullptr;
	
	// 인벤토리 슬롯 선택 시 이미지
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Inventory", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UTexture2D> FocusSlotImage = nullptr;
	
	// 소모품 슬롯 이미지
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Inventory", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UTexture2D> BasicConsumableSlotImage = nullptr;
	
	// 소모품 슬롯 선택 시 이미지
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Inventory", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UTexture2D> FocusConsumableSlotImage = nullptr;
	
	// 인벤토리 열 수
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Inventory", meta=(AllowPrivateAccess="true"))
	int32 ColumnSize = 4;
		
	// 인벤토리 슬롯 배열
	UPROPERTY(Transient)
	TArray<TObjectPtr<UERNInventorySlotWidget>> SlotWidgets;
	
	// 현재 바인딩된 인벤토리 컴포넌트
	UPROPERTY(Transient)
	TWeakObjectPtr<UERNInventoryComponent> BoundInventoryComponent;
	
	// 현재 바인딩된 장착 컴포넌트
	UPROPERTY(Transient)
	TWeakObjectPtr<UERNEquipmentComponent> BoundEquipmentComponent;
	
	// 활성화된 슬롯 인덱스 (선택, 고정)
	int32 FocusSlotIndex = -1;
	
	// 임시 포커스된 슬롯 인덱스 (Hover)
	int32 HoveredSlotIndex = -1;
	
};
