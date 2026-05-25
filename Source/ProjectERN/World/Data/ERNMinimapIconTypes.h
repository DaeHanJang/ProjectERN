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
	PlayerPin1	UMETA(DisplayName = "PlayerPin1"),	
	PlayerPin2	UMETA(DisplayName = "PlayerPin2"),	
	PlayerPin3	UMETA(DisplayName = "PlayerPin3"),	
};