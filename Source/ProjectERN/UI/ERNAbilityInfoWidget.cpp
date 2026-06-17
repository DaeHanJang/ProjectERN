#include "UI/ERNAbilityInfoWidget.h"
#include "Components/TextBlock.h"

void UERNAbilityInfoWidget::SetAbilityText(const FText& InText)
{
	if (IsValid(AbilityText))
	{
		AbilityText->SetText(InText);
	}
}
