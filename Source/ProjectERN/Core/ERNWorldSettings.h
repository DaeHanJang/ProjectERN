// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/WorldSettings.h"
#include "ERNWorldSettings.generated.h"

class USoundBase;

/**
 * 프로젝트 전용 WorldSettings.
 * 맵별 데이터를 World Settings 패널에서 직접 설정.
 *
 * Project Settings → Maps & Modes → World Settings Class 에 이 클래스 지정.
 */
UCLASS()
class PROJECTERN_API AERNWorldSettings : public AWorldSettings
{
	GENERATED_BODY()

public:
	// 맵 진입 시 자동 재생할 BGM (SoundSubsystem이 OnPostLoadMapWithWorld에서 자동 호출)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound|BGM")
	TObjectPtr<USoundBase> MapBGM;

	// BGM 페이드 인 시간 (초)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Sound|BGM", meta = (ClampMin = "0.0"))
	float MapBGMFadeInTime = 2.0f;
};
