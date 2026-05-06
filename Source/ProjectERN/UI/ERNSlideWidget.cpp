// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/ERNSlideWidget.h"

#include "Components/Button.h"
#include "Components/EditableText.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"

UERNSlideWidget::UERNSlideWidget(const FObjectInitializer& ObjectInitializer)  : Super(ObjectInitializer)
{
	SetIsFocusable(true);
}

void UERNSlideWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	SetVisibility(ESlateVisibility::Hidden);
	
	SlideWidgetEditableText->OnTextChanged.AddDynamic(this, &UERNSlideWidget::HandleEditableTextChanged);
	SlideWidgetEditableText->OnTextCommitted.AddDynamic(this, &UERNSlideWidget::HandleEditableTextCommitted);
	SlideWidgetSlider->OnValueChanged.AddDynamic(this, &UERNSlideWidget::HandleSlider);
	ConfirmButton->OnClicked.AddDynamic(this, &UERNSlideWidget::HandleConfirmButtonClicked);
	CancelButton->OnClicked.AddDynamic(this, &UERNSlideWidget::HandleCancelButtonClicked);
}

void UERNSlideWidget::InitSlideWidget(const FString& Text, const int32 MaxValue) const
{
	SlideWidgetTextBlock->SetText(FText::FromString(Text));
	SlideWidgetEditableText->SetText(FText::AsNumber(0));
	MaxTextBlock->SetText(FText::AsNumber(MaxValue));
	SlideWidgetSlider->SetValue(0.0f);
}

void UERNSlideWidget::HandleEditableTextChanged(const FText& Text)
{
	const int32 NewValue = FCString::Atoi(*Text.ToString());
	const int32 MaxValue = FCString::Atoi(*MaxTextBlock->GetText().ToString());
	// 슬라이더 갱신
	SlideWidgetSlider->SetValue(NewValue / MaxValue);
	Value = NewValue;
}

void UERNSlideWidget::HandleEditableTextCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
	if (CommitMethod == ETextCommit::OnEnter || CommitMethod == ETextCommit::OnUserMovedFocus)
	{
		const int32 NewValue = FCString::Atoi(*Text.ToString());
		const int32 MaxValue = FCString::Atoi(*MaxTextBlock->GetText().ToString());
		// 슬라이더 갱신
		SlideWidgetSlider->SetValue(NewValue / MaxValue);
		Value = NewValue;
	}
	else if (CommitMethod == ETextCommit::OnCleared)
	{
		SlideWidgetSlider->SetValue(0.0f);
		Value = 0;
	}
}

void UERNSlideWidget::HandleSlider(float InValue)
{
	const int32 MaxValue = FCString::Atoi(*MaxTextBlock->GetText().ToString());
	const int32 NewValue = static_cast<int32>(InValue * MaxValue);
	// 값 입력 텍스트 갱신
	SlideWidgetEditableText->SetText(FText::AsNumber(static_cast<int32>(NewValue)));
	Value = NewValue;
}

void UERNSlideWidget::HandleConfirmButtonClicked()
{
	OnConfirmButtonClicked.Broadcast(Value);
}

void UERNSlideWidget::HandleCancelButtonClicked()
{
	Value = 0;
	OnCancelButtonClicked.Broadcast(Value);
}
