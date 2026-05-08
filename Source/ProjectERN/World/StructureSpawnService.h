// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "World/Data/EStructureType.h"
#include "UObject/Object.h"
#include "StructureSpawnService.generated.h"

struct FERNStructureSpawnEntry;
class AStructureSpawnPoint;
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
	
private:
	// 스폰 가능 지점 수집
	void CollectSpawnPoints(UWorld* World);
	
	// 건물별 최소 스폰 개수만큼 SpawnEntryStructures 호출
	void SpawnMinimumStructures(UWorld* World, const UStructureSpawnConfigDataAsset* SpawnConfig);
	
	// 최소 스폰 이후 SpawnEntryStructures 추가 호출
	void SpawnAdditionalStructures(UWorld* World, const UStructureSpawnConfigDataAsset* SpawnConfig);
	
	// 건물 스폰
	int32 SpawnEntryStructures(UWorld* World, const FERNStructureSpawnEntry& Entry, int32 DesiredSpawnCount);
	
	bool TrySpawnStructureAtPoint(UWorld* World, UClass* PackedClass, const AStructureSpawnPoint* SpawnPoint);
	
	// 스폰 가능한 랜덤 지점 추출
	AStructureSpawnPoint* PopRandomSpawnPoint(EStructureType SpawnType);
	
private:
	TMap<EStructureType, TArray<AStructureSpawnPoint*>> SpawnPoints;
};
