// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/ERNFinScreenWidget.h"

#include "Components/TextBlock.h"
#include "Animation/WidgetAnimation.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"

UERNFinScreenWidget::UERNFinScreenWidget(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	SetIsFocusable(true);
}

void UERNFinScreenWidget::NativeConstruct()
{
	Super::NativeConstruct();

	bSpaceEnabled = false;
	bContinued = false;

	// (?) / Space 프롬프트는 시작 시 숨김
	if (QuestionText)
	{
		QuestionText->SetVisibility(ESlateVisibility::Hidden);
	}
	if (SpacePromptText)
	{
		SpacePromptText->SetVisibility(ESlateVisibility::Hidden);
	}

	// Space 키 입력 수신을 위해 입력 모드(UI 허용) + 포커스 확보
	// (BP에서 별도 입력 모드 설정 없이 CreateWidget + AddToViewport만으로 동작하도록)
	if (APlayerController* PC = GetOwningPlayer())
	{
		FInputModeGameAndUI InputMode;
		InputMode.SetWidgetToFocus(TakeWidget());
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(InputMode);
	}
	SetKeyboardFocus();

	// 1단계: "Fin" 페이드 인
	if (FinFadeIn)
	{
		FWidgetAnimationDynamicEvent FinFinished;
		FinFinished.BindDynamic(this, &UERNFinScreenWidget::HandleFinFadeFinished);
		BindToAnimationFinished(FinFadeIn, FinFinished);
		PlayAnimation(FinFadeIn);
	}
	else
	{
		// 애니메이션이 없으면 바로 다음 단계로 진행 (진행 막힘 방지)
		HandleFinFadeFinished();
	}
}

void UERNFinScreenWidget::HandleFinFadeFinished()
{
	// 2단계: "(?)" 페이드 인
	if (QuestionText)
	{
		QuestionText->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	if (QuestionFadeIn)
	{
		FWidgetAnimationDynamicEvent QuestionFinished;
		QuestionFinished.BindDynamic(this, &UERNFinScreenWidget::HandleQuestionFadeFinished);
		BindToAnimationFinished(QuestionFadeIn, QuestionFinished);
		PlayAnimation(QuestionFadeIn);
	}
	else
	{
		HandleQuestionFadeFinished();
	}
}

void UERNFinScreenWidget::HandleQuestionFadeFinished()
{
	// 3단계: Space 프롬프트 깜빡임 + 입력 활성화
	StartSpacePrompt();
}

void UERNFinScreenWidget::StartSpacePrompt()
{
	bSpaceEnabled = true;

	if (SpacePromptText)
	{
		SpacePromptText->SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	// 무한 루프(NumLoopsToPlay = 0)로 깜빡임 재생
	if (SpaceBlink)
	{
		PlayAnimation(SpaceBlink, 0.0f, 0);
	}

	// 애니메이션 진행 중 포커스가 풀렸을 수 있으므로 재확보
	SetKeyboardFocus();
}

FReply UERNFinScreenWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (bSpaceEnabled && InKeyEvent.GetKey() == EKeys::SpaceBar)
	{
		Continue();
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UERNFinScreenWidget::Continue()
{
	if (bContinued)
	{
		return;
	}
	bContinued = true;

	if (SpaceBlink)
	{
		StopAnimation(SpaceBlink);
	}

	// PC가 바인딩 → 승리 배너(WBP_Victory) 표시
	OnContinue.Broadcast();

	RemoveFromParent();
}
