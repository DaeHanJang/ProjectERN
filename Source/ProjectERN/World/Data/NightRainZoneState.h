// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "NightRainZoneState.generated.h"

/**
 * 
 */

USTRUCT(BlueprintType)
struct PROJECTERN_API FNightRainZoneState
{
	GENERATED_BODY()

	// 자기장이 활성화 된 이후부터는 데미지 판정은 계속 진행됨. bRunning
	UPROPERTY(BlueprintReadOnly)
	bool bShrinking = false;
	
	UPROPERTY(BlueprintReadOnly)
	FVector StartCenter = FVector::ZeroVector;
	
	UPROPERTY(BlueprintReadOnly)
	FVector TargetCenter = FVector::ZeroVector;
	
	UPROPERTY(BlueprintReadOnly)
	float StartRadius = 0.f;
	
	UPROPERTY(BlueprintReadOnly)
	float TargetRadius = 0.f;
	
	UPROPERTY(BlueprintReadOnly)
	double PhaseStartServerWorldTimeSeconds = 0.0;
	
	UPROPERTY(BlueprintReadOnly)
	float PhaseDuration = 0.f;
	
	UPROPERTY(BlueprintReadOnly)
	float FreezingDuration = 0.f;
	
	UPROPERTY(BlueprintReadOnly)
	int32 PhaseIndex = 0;
	
	UPROPERTY(BlueprintReadOnly)
	int32 Revision = 0;
	
	// 자기장 진행 일시정지 유무
	UPROPERTY(BlueprintReadOnly)
	bool bProgressPaused = false;
	
	// 일시정지 상태의 수렴 진행도
	UPROPERTY(BlueprintReadOnly)
	float PausedAlpha = 0.f;
	
	//진행도
	float GetAlpha(double ServerTime) const
	{
		// 일시정지라면 계산 없이 즉시 멈춘 시점의 값 반환
		if (bProgressPaused)
		{
			return PausedAlpha;
		}
		
		if (PhaseDuration <= 0.f)
		{
			return 1.f;
		}
		
		return FMath::Clamp(static_cast<float>((ServerTime - PhaseStartServerWorldTimeSeconds) / PhaseDuration), 0.f, 1.f);
	}
	
	
	FVector GetCenterAtTime(double ServerTime) const
	{
		return FMath::Lerp(StartCenter, TargetCenter, GetAlpha(ServerTime));
	}
	
	float GetRadiusAtTime(double ServerTime) const
	{
		return FMath::Lerp(StartRadius, TargetRadius, GetAlpha(ServerTime));
	}
};