// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ERNInventorySlotWidget.h"
#include "Blueprint/UserWidget.h"
#include "Inventory/Data/ERNInventoryList.h"
#include "ERNInventoryWidget.generated.h"

class UERNEquipmentComponent;
class UERNSlideWidget;
class UUniformGridPanel;
class UERNInventorySlotWidget;

/**
 * 
 */
UCLASS()
class PROJECTERN_API UERNInventoryWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UERNInventoryWidget(const FObjectInitializer& ObjectInitializer);
	
	// 활성화된 슬롯 인덱스 초기화
	FORCEINLINE void InitFocusSlotIndex()
	{
		UpdateFocusSlotIndex(-1);
	}
	
	// 슬롯 생성
	UFUNCTION(BlueprintCallable, Category="InventoryUI")
	void CreateSlot(const int32 MaxSlotSize, const int32 ColumnCount);
	
protected:
	virtual void NativeConstruct() override;
	
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	
private:
	// 인벤토리 컴포넌트 가져오기
	UERNInventoryComponent* GetInventoryComponent() const;
	// 장착 컴포넌트 가져오기
	UERNEquipmentComponent* GetEquipmentComponent() const;
	
	// 인벤토리 내비게이션 처리
	const int32 GetNavigationTargetSlotIndex(const FKey& Key, const int32 MaxSlotSize) const;
	
	// 인벤토리 슬롯 갱신 이벤트 핸들러
	UFUNCTION()
	void UpdateInventorySlot(const FInventoryItemEntry& Entry);
	
	// 장비 슬롯 갱신 이벤트 핸들러
	UFUNCTION()
	void UpdateEquipmentSlot(const FInventoryItemEntry& Entry);
	
	// 슬롯 활성화 이벤트 핸들러
	UFUNCTION()
	void UpdateFocusSlotIndex(const int32 NewIndex);
	
	// 슬라이어 위젯 갱신 이벤트 핸들러
	UFUNCTION()
	void UpdateSlideWidget(const int32 NewQuantity);
	
private:
	// 장비 슬롯
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UERNInventorySlotWidget> EquipableSlotWidget;
	
	// 소모품 슬롯
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UERNInventorySlotWidget> ConsumableSlotWidget;
	
	// 인벤토리
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UUniformGridPanel> InventoryUniformGridPanel;
	
	// 슬라이드 위젯
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UERNSlideWidget> WBP_SlideWidget;
	
	// 인벤토리 슬롯 위젯 클래스
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Inventory", meta=(AllowPrivateAccess="true"))
	TSubclassOf<UUserWidget> SlotWidgetClass;
	
	// 슬롯 이미지
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Inventory", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UTexture2D> BasicSlotImage = nullptr;
	
	// 슬롯 선택 시 이미지
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Inventory", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UTexture2D> FocusSlotImage = nullptr;
	
	// 인벤토리 열 수
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Inventory", meta=(AllowPrivateAccess="true"))
	int32 ColumnSize = 4;
	
	// 인벤토리 슬롯 배열
	UPROPERTY(Transient)
	TArray<TObjectPtr<UERNInventorySlotWidget>> SlotWidgets;
	
	// 활성화된 슬롯 인덱스
	int32 FocusSlotIndex = -1;
	
};
