// Fill out your copyright notice in the Description page of Project Settings.


#include "Input/ERNInputConfig.h"

const UInputAction* UERNInputConfig::FindNativeInputActionByTag(const FGameplayTag& InputTag) const
{
	for (const FERNInputAction& Action : NativeInputActions)
	{
		if (Action.InputAction && Action.InputTag == InputTag)
		{
			return Action.InputAction;
		}
	}

	return nullptr;
}
