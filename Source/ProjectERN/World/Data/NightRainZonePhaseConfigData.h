// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "NightRainZonePhaseConfigData.generated.h"

/**
 * 
 */

//에디터에서 설정할 초기 설정값
USTRUCT(BlueprintType)
struct FNightRainZonePhaseConfig
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FVector StartCenter = FVector::ZeroVector;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FVector TargetCenter = FVector::ZeroVector;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0"))
	float StartRadius = 100000.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0"))
	float TargetRadius = 20000.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.01"))
	float Duration = 180.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0"))
	float FreezingDuration = 300.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0"))
	float DamagePerTick = 10.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.01"))
	float DamageTickInterval = 1.f;
};

UCLASS()
class PROJECTERN_API UNightRainZonePhaseConfigData : public UDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, Category="Night Rain Zone")
	FNightRainZonePhaseConfig InitPhaseConfig;
	
	UPROPERTY(EditAnywhere, Category="Night Rain Zone")
	TArray<FNightRainZonePhaseConfig> ShrinkPhaseConfigs;
};
