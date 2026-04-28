// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ERNInventorySlotWidget.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTERN_API UERNInventorySlotWidget : public UUserWidget
{
	GENERATED_BODY()
	
private:
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UImage> ItemImage;
	
};
