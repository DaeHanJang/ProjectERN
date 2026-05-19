// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "World/Data/MobRuntimeState.h"
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ERNWorldManagerSubsystem.generated.h"

class UStructureSpawnService;
class UStructureSpawnConfigDataAsset;
class UERNDayNightCycleConfigDataAsset;
class UERNDayNightCycleService;

struct FERNWorldTableRow;

/**
 * 
 */
UCLASS()
class PROJECTERN_API UERNWorldManagerSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()
	
public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	
public:
	bool TryGetMobStates(FName SpawnerId, TArray<FMobRuntimeState>& OutSavedStates) const;
	void SaveMobStates(FName SpawnerId, const TArray<FMobRuntimeState>& InStates);
	
private:
	bool TryFindWorldRow(const UWorld* World, FERNWorldTableRow& OutWorldRow, FString* OutWorldPath) const;
	
	FString CurrentWorldPath;
	
	// 구조물 스폰 설정 데이터 에셋
	UPROPERTY()
	TSoftObjectPtr<UStructureSpawnConfigDataAsset> CachedSpawnConfig;
	
	// 구조물 스폰 시스템
	UPROPERTY()
	TObjectPtr<UStructureSpawnService> StructureSpawnService;
	
	// 낮밤 설정 데이터 에셋
	UPROPERTY()
	TSoftObjectPtr<UERNDayNightCycleConfigDataAsset> CachedDayNightConfig;
	
	// 낮밤 시스템
	UPROPERTY()
	TObjectPtr<UERNDayNightCycleService> DayNightCycleService;
	
	// 몹 스포너 데이터 저장소
	UPROPERTY()
	TMap<FName, FSpawnerMobRuntimeStates> SpawnerMobStates;
	
	
};
