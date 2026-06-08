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
	RewardButton->OnClicked.AddUniqueDynamic(this, &UDHRollWidget::OnReward);
}

void UDHRollWidget::NativeDestruct()
{
	RollButton->OnClicked.RemoveDynamic(this, &UDHRollWidget::OnRoll);
	RewardButton->OnClicked.RemoveDynamic(this, &UDHRollWidget::OnReward);
	
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
	if (OnRewardClicked.IsBound())
	{
		OnRewardClicked.Broadcast(GetOwningPlayer());
	}
}

void UDHRollWidget::SetText(const int32 Grade) const
{
	if (Grade == 0)
	{
		TextBlock->SetText(FText::FromString(TEXT("70%의 확률로 5000골드를 획득합니다.\n실패 시 5000골드를 회수합니다.")));
	}
	else if (Grade == 1)
	{
		TextBlock->SetText(FText::FromString(TEXT("50%의 확률로 20000골드를 획득합니다.\n실패 시 20000골드를 회수합니다.")));
	}
	else if (Grade == 2)
	{
		TextBlock->SetText(FText::FromString(TEXT("25%의 확률로 전설 등급 무기를 획득합니다.\n실패 시 모든 골드를 회수합니다.")));
	}
	else if (Grade == 3)
	{
		TextBlock->SetText(FText::FromString(TEXT("전설 무기를 획득하셨습니다! 보상을 받으세요!")));
	}
	else if (Grade == 4)
	{
		TextBlock->SetText(FText::FromString(TEXT("하나뿐인 기회를 놓치셨습니다..")));
	}
	else if (Grade == 5)
	{
		TextBlock->SetText(FText::FromString(TEXT("5000골드를 획득하셨습니다.")));
	}
	else if (Grade == 6)
	{
		TextBlock->SetText(FText::FromString(TEXT("20000골드를 획득하셨습니다.")));
	}
	else if (Grade == -1)
	{
		TextBlock->SetText(FText::FromString(TEXT("실패.. 5000골드를 회수합니다.")));
	}
	else if (Grade == -2)
	{
		TextBlock->SetText(FText::FromString(TEXT("실패.. 20000골드를 회수합니다.")));
	}
	else if (Grade == -3)
	{
		TextBlock->SetText(FText::FromString(TEXT("실패.. 모든 골드를 회수합니다.")));
	}
}

void UDHRollWidget::SetVisibilityRollButton(const bool bVisibility) const
{
	if (bVisibility)
	{
		RollButton->SetVisibility(ESlateVisibility::Visible);
	}
	else
	{
		RollButton->SetVisibility(ESlateVisibility::Hidden);
	}
}

void UDHRollWidget::SetVisibilityRewardButton(const bool bVisibility) const
{
	if (bVisibility)
	{
		RewardButton->SetVisibility(ESlateVisibility::Visible);
	}
	else
	{
		RewardButton->SetVisibility(ESlateVisibility::Hidden);
	}
}
