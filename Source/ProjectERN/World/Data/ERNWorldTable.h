// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "ERNWorldTable.generated.h"

/**
 * 
 */

class UStructureSpawnConfigDataAsset;
class UERNDayNightCycleConfigDataAsset;

USTRUCT(BlueprintType)
struct PROJECTERN_API FERNWorldTableRow : public FTableRowBase
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftObjectPtr<UWorld> World;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftObjectPtr<UStructureSpawnConfigDataAsset> SpawnConfig;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftObjectPtr<UERNDayNightCycleConfigDataAsset> DayNightConfig;
};
