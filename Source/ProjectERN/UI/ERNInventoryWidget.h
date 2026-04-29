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
	
protected:
	virtual void NativeConstruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
		
private:
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UERNInventorySlotWidget> EquipableSlotWidget;
	
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UERNInventorySlotWidget> ConsumableSlotWidget;
	
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UUniformGridPanel> InventoryUniformGridPanel;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Inventory", meta=(AllowPrivateAccess="true"))
	TSubclassOf<UUserWidget> SlotWidgetClass;
	
	UPROPERTY()
	TArray<TObjectPtr<UERNInventorySlotWidget>> SlotWidgets;
	
};
