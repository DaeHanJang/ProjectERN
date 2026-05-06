// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ERNInteractableWidget.generated.h"


// 닫기 이벤트를 액터에게 전달할 델리게이트
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWidgetClosedSignature);
/**
 * 
 */
UCLASS()
class PROJECTERN_API UERNInteractableWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	UERNInteractableWidget(const FObjectInitializer& ObjectInitializer);
	
	UPROPERTY(BlueprintAssignable, Category = "UI|Interaction")
	FOnWidgetClosedSignature OnWidgetClosed;

protected:
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	
};
