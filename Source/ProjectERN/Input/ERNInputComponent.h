#pragma once

#include "CoreMinimal.h"
#include "EnhancedInputComponent.h"
#include "Input/ERNInputConfig.h"
#include "ERNInputComponent.generated.h"

UCLASS()
class PROJECTERN_API UERNInputComponent : public UEnhancedInputComponent
{
	GENERATED_BODY()

public:
	template <class UserObject, typename CallbackFunc>
	void BindNativeInputAction(
		const UERNInputConfig* InputConfig,
		const FGameplayTag& InputTag,
		ETriggerEvent TriggerEvent,
		UserObject* ContextObject,
		CallbackFunc Func);
};

template <class UserObject, typename CallbackFunc>
void UERNInputComponent::BindNativeInputAction(
	const UERNInputConfig* InputConfig,
	const FGameplayTag& InputTag,
	ETriggerEvent TriggerEvent,
	UserObject* ContextObject,
	CallbackFunc Func)
{
	checkf(InputConfig, TEXT("ERNInputComponent: InputConfig is null."));

	if (const UInputAction* InputAction = InputConfig->FindNativeInputActionByTag(InputTag))
	{
		BindAction(InputAction, TriggerEvent, ContextObject, Func);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Failed to find InputAction for InputTag [%s] in InputConfig [%s]."),
			*InputTag.ToString(),
			*InputConfig->GetName());
	}
}
