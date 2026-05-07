// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "StructureSpawnService.generated.h"

class UStructureSpawnConfigDataAsset;
/**
 * 
 */
UCLASS()
class PROJECTERN_API UStructureSpawnService : public UObject
{
	GENERATED_BODY()
	
public:
	void SpawnStructures(UWorld* World, const UStructureSpawnConfigDataAsset* SpawnConfig);
};
