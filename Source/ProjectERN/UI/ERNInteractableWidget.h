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
	
	// 블루프린트에서 닫기 애니메이션을 재생하도록 구현할 이벤트
	UFUNCTION(BlueprintNativeEvent, Category = "UI|Animation")
	void BP_PlayCloseAnimation();
	
	// 블루프린트에서 애니메이션이 끝나면 C++로 완료를 알릴 함수
	UFUNCTION(BlueprintCallable, Category = "UI|Animation")
	void NotifyCloseAnimationFinished();
	
protected:
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	
};
