// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Image.h"
#include "ERNInventorySlotWidget.generated.h"

class UERNInventoryWidget;
class UTextBlock;
class UImage;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSlotClicked, const int32, Index);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSlotHovered, const int32, Index);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSlotUnhovered, const int32, Index);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSlotDoubleClicked, const int32, Index);

/**
 * 
 */
UCLASS()
class PROJECTERN_API UERNInventorySlotWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UERNInventorySlotWidget(const FObjectInitializer& ObjectInitializer);
	
	// Getter/Setter
	FORCEINLINE const int32 GetSlotIndex() const { return SlotIndex; }
	FORCEINLINE void SetSlotIndex(const int32 NewIndex) { SlotIndex = NewIndex; }
	FORCEINLINE void SetBackgroundTint(FColor NewColor) { BackgroundTint = NewColor; }
	FORCEINLINE void InitInventorySlotTint() const { if (InventorySlotImage) InventorySlotImage->SetBrushTintColor(BackgroundTint); }
	void SetInventorySlotImage(UTexture2D* NewTexture) const;
	void SetInventorySlotTint(FColor NewColor) const;
	
	// Clear UI
	UFUNCTION(BlueprintCallable)
	void ClearItem();
	
	// Set UI
	UFUNCTION(BlueprintCallable)
	void SetItem(UTexture2D* Icon, const int32 QuantityText, FColor Color);
	
protected:
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonDoubleClick(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;
	
public:
	// Click Event
	UPROPERTY(Transient)
	FOnSlotClicked OnSlotClicked;

	UPROPERTY(Transient)
	FOnSlotHovered OnSlotHovered;
	
	UPROPERTY(Transient)
	FOnSlotUnhovered OnSlotUnhovered;
	
	UPROPERTY(Transient)
	FOnSlotDoubleClicked OnSlotDoubleClicked;
	
private:
	// Slot Image
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UImage> InventorySlotImage;
	
	// Icon Image
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UImage> ItemImage;
	
	// Quantity Text
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> ItemQuantityTextBlock;
	
	// Slot Index
	int32 SlotIndex = -1;
	
	// BackgroundImage Tint Color
	FColor BackgroundTint = FColor::White;
	
};
