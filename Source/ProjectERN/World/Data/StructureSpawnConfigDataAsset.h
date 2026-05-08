// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "EStructureType.h"
#include "Engine/DataAsset.h"
#include "PackedLevelActor/PackedLevelActor.h"
#include "StructureSpawnConfigDataAsset.generated.h"


USTRUCT(BlueprintType)
struct FERNStructureSpawnEntry
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EStructureType SpawnType = EStructureType::None;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftClassPtr<APackedLevelActor> PackedLevelActorClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", UIMin="0"))
	int32 MinSpawnCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", UIMin="0"))
	int32 MaxSpawnCount = 1;
};

/**
 * 
 */
UCLASS()
class PROJECTERN_API UStructureSpawnConfigDataAsset : public UDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FERNStructureSpawnEntry> SpawnEntries;
};
