// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ERNItemAssetLoadTypes.generated.h"

// Soft reference 로드 정책 플래그
UENUM(BlueprintType, meta=(Bitflags))
enum class EItemAssetLoadFlags : uint8
{
	None     = 0,
	UI       = 1 << 0,
	Gameplay = 1 << 1,
	All      = UI | Gameplay
};
ENUM_CLASS_FLAGS(EItemAssetLoadFlags)
