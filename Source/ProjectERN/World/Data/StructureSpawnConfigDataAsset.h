// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "PackedLevelActor/PackedLevelActor.h"
#include "StructureSpawnConfigDataAsset.generated.h"


USTRUCT(BlueprintType)
struct FERNStructureSpawnEntry
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftClassPtr<APackedLevelActor> PackedLevelActorClass;
	
	//UPROPERTY(EditAnywhere, BlueprintReadWrite)
	//TSoftObjectPtr<UStaticMesh> StaticMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float SpawnWeight = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MinSpawnCount = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
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
