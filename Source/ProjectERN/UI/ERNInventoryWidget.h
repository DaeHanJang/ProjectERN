// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ERNInventoryWidget.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTERN_API UERNInventoryWidget : public UUserWidget
{
	GENERATED_BODY()

private:
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UEquipableSlotWidget> EquipableSlotWidget;
	
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UConsumableSlotWidget> ConsumableSlotWidget;
	
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UUniformGridPanel> InventoryUniformGridPanel;
	
};
