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
	
	//0번째 페이즈에서는 ZeroVector. 이후부터는 월드에 배치된 BP_NightRainZoneCenterPoint 로 설정됨
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FVector TargetCenter = FVector::ZeroVector;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0"))
	float TargetRadius = 100000.f;
	
	// 자기장 축소 시간
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.01"))
	float ShrinkDuration = 180.f;
	
	// 자기장 축소 후 대기 시간
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0"))
	float FreezingDuration = 300.f;
	
	// 틱당 데미지
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0"))
	float DamagePerTick = 10.f;
	
	// 데미지 틱 간격
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.01"))
	float DamageTickInterval = 1.f;
};

UCLASS()
class PROJECTERN_API UNightRainZonePhaseConfigData : public UDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, Category="Night Rain Zone")
	TArray<FNightRainZonePhaseConfig> PhaseConfigs;
};
