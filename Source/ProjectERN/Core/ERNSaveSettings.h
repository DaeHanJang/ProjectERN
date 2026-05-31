// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "ERNSaveSettings.generated.h"

UCLASS()
class PROJECTERN_API UERNSaveSettings : public USaveGame
{
	GENERATED_BODY()

public:
	// Audio (0.0 ~ 1.0)
	UPROPERTY(BlueprintReadWrite, Category = "Audio")
	float MasterVolume = 1.0f;

	UPROPERTY(BlueprintReadWrite, Category = "Audio")
	float MusicVolume = 1.0f;

	UPROPERTY(BlueprintReadWrite, Category = "Audio")
	float SFXVolume = 1.0f;

	// Brightness (UGameUserSettings에 없어서 별도 저장, 0 ~ 100)
	UPROPERTY(BlueprintReadWrite, Category = "Video")
	float Brightness = 50.0f;
};
