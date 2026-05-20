// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/TargetPoint.h"
#include "NightRainZoneCenterPoint.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTERN_API ANightRainZoneCenterPoint : public ATargetPoint
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere)
	bool bEnabled = true;
	
	UPROPERTY(EditAnywhere)
	int32 ZoneLevel = 0;
};
