// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/ERNInteractableWidget.h"
#include "InputCoreTypes.h"

UERNInteractableWidget::UERNInteractableWidget(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	// 위젯이 키입력을 발을 수 있도록 포커스 불리언 활성화
	bIsFocusable = true;
}

FReply UERNInteractableWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape)
	{
		// 닫힘 이벤트를 브로드 캐스트 신호로 쏴주기
		OnWidgetClosed.Broadcast();

		// 입력이 처리되었음을 언리얼 엔진에 알림
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry,InKeyEvent);
}
