// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ERNInventorySlotWidget.generated.h"

class UTextBlock;
class UImage;

/**
 * 
 */
UCLASS()
class PROJECTERN_API UERNInventorySlotWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	// Clear UI
	UFUNCTION(BlueprintCallable)
	void ClearItem() const;
	
	// Set UI
	UFUNCTION(BlueprintCallable)
	void SetItem(UTexture2D* Icon, int32 QuantityText) const;
	
private:
	// Icon Image
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UImage> ItemImage;
	
	// Quantity Text
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> ItemQuantityTextBlock; 
	
};
