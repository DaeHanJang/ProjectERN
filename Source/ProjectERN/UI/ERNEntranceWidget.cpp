// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/ERNEntranceWidget.h"

#include "Components/TextBlock.h"

void UERNEntranceWidget::SetEntranceText(const FText& InText)
{
	if (EntranceTextBlock)
	{
		EntranceTextBlock->SetText(InText);
	}
}
