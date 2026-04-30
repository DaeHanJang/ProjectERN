// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ERNInventorySlotWidget.h"
#include "Blueprint/UserWidget.h"
#include "Inventory/Data/ERNInventoryList.h"
#include "ERNInventoryWidget.generated.h"

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
	
	UFUNCTION()
	void HandleInventorySlotChanged(const FInventoryItemEntry& Entry);
	
	UFUNCTION(BlueprintCallable, Category="InventoryUI")
	void CreateSlot(int32 MaxSlotSize, int32 ColumnCount);
	
	void InitFocusSlotIndex() { FocusSlotIndex = -1; }
	
protected:
	virtual void NativeConstruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	
private:
	UFUNCTION()
	void SetFocusSlotIndex(const int32 NewIndex);
	
	UERNInventoryComponent* GetInventoryComponent() const;
	
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
	
	// 인벤토리 슬롯 위젯 클래스
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Inventory", meta=(AllowPrivateAccess="true"))
	TSubclassOf<UUserWidget> SlotWidgetClass;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Inventory", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UTexture2D> BasicSlotImage = nullptr;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Inventory", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UTexture2D> FocusSlotImage = nullptr;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Inventory", meta=(AllowPrivateAccess="true"))
	int32 ColumnSize = 4;
	
	// 인벤토리 슬롯 배열
	UPROPERTY(Transient)
	TArray<TObjectPtr<UERNInventorySlotWidget>> SlotWidgets;
	
	// 활성화된 슬롯 인덱스
	int32 FocusSlotIndex = -1;
	
};
