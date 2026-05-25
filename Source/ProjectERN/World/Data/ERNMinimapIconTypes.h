#pragma once

#include "CoreMinimal.h"
#include "ERNMinimapIconTypes.generated.h"

UENUM(BlueprintType)
enum class EERNMinimapIconType : uint8
{
	Building	UMETA(DisplayName = "Building"),
	JumpPad		UMETA(DisplayName = "JumpPad"),
	SpiritBird	UMETA(DisplayName = "SpiritBird"),
	Shop		UMETA(DisplayName = "Shop"),
	Church		UMETA(DisplayName = "Church"),
	Castle		UMETA(DisplayName = "Castle"),
	Player		UMETA(DisplayName = "Player"),
};