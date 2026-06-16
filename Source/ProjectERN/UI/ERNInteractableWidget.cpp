// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/ERNInteractableWidget.h"
#include "InputCoreTypes.h"

UERNInteractableWidget::UERNInteractableWidget(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	// 위젯이 키입력을 발을 수 있도록 포커스 불리언 활성화
	SetIsFocusable(true);
}

FReply UERNInteractableWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape || InKeyEvent.GetKey() == EKeys::Gamepad_FaceButton_Right)
	{
		// 즉시 닫지 않고 블루프린트의 닫기 애니메이션 재생 이벤트를 호출
		BP_PlayCloseAnimation();

		// 입력이 처리되었음을 언리얼 엔진에 알림
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry,InKeyEvent);
}

void UERNInteractableWidget::NotifyCloseAnimationFinished()
{
	OnWidgetClosed.Broadcast();
}

void UERNInteractableWidget::BP_PlayCloseAnimation_Implementation()
{
	// 블루프린트에서 구현하지 않았을 경우의 기본 동작: 애니메이션 없이 즉시 닫힘 처리
	NotifyCloseAnimationFinished();
}
