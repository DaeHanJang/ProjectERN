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

	// DLSS 사용 여부 (체크박스)
	UPROPERTY(BlueprintReadWrite, Category = "Video")
	bool bDLSSEnabled = false;

	// DLSS 품질 모드 (UDLSSMode 값, 4 = Quality) — 플러그인 의존 피하려고 int32로 저장
	UPROPERTY(BlueprintReadWrite, Category = "Video")
	int32 DLSSMode = 4;

	// 마우스 감도 슬라이더 값 (0.0 ~ 1.0, 0.5 = 기본 1.0배). 실제 배율은 GameInstance에서 매핑
	UPROPERTY(BlueprintReadWrite, Category = "Input")
	float MouseSensitivity = 0.5f;
};
