// Copyright Epic Games, Inc. All Rights Reserved.

#include "UI/ERNDamageTextWidget.h"
#include "Components/TextBlock.h"

void UERNDamageTextWidget::SetDamageText(float Damage, FLinearColor Color)
{
	if (DamageText)
	{
		// 정수로 표시
		DamageText->SetText(FText::AsNumber(FMath::RoundToInt(Damage)));
		DamageText->SetColorAndOpacity(FSlateColor(Color));
	}
}
