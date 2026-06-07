// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/DHRollWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"

UDHRollWidget::UDHRollWidget(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

void UDHRollWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	RollButton->OnClicked.AddUniqueDynamic(this, &UDHRollWidget::OnRoll);
	RewordButton->OnClicked.AddUniqueDynamic(this, &UDHRollWidget::OnReward);
}

void UDHRollWidget::NativeDestruct()
{
	RollButton->OnClicked.RemoveDynamic(this, &UDHRollWidget::OnRoll);
	RewordButton->OnClicked.RemoveDynamic(this, &UDHRollWidget::OnReward);
	
	Super::NativeDestruct();
}

void UDHRollWidget::OnRoll()
{
	if (OnRollClicked.IsBound())
	{
		OnRollClicked.Broadcast(GetOwningPlayer());
	}
}

void UDHRollWidget::OnReward()
{
	if (OnRewordClicked.IsBound())
	{
		OnRewordClicked.Broadcast(GetOwningPlayer());
	}
}

void UDHRollWidget::SetText(const FString& Text) const
{
	TextBlock->SetText(FText::FromString(Text));
}
