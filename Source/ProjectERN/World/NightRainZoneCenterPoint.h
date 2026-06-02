// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/TargetPoint.h"
#include "NightRainZoneCenterPoint.generated.h"

class AMiddleBossSpawner;
/**
 * 
 */
UCLASS()
class PROJECTERN_API ANightRainZoneCenterPoint : public ATargetPoint
{
	GENERATED_BODY()

public:
	UFUNCTION()
	void HandleZoneShrinkFinished();
	
public:
	UPROPERTY(EditAnywhere, Category = "Night Rain Zone")
	bool bEnabled = true;
	
	UPROPERTY(EditAnywhere, Category = "Night Rain Zone")
	int32 ZoneLevel = 0;

private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MibbleBossSpawner", meta = (AllowPrivateAccess = true))
	TObjectPtr<AMiddleBossSpawner> MiddleBossSpawner;
};
