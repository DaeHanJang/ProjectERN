// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ERNWorldManagerSubsystem.generated.h"


struct FERNWorldTableRow;
class UStructureSpawnService;
class UStructureSpawnConfigDataAsset;
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

private:
	bool TryFindWorldRow(const UWorld* World, FERNWorldTableRow& OutWorldRow, FString* OutWorldPath) const;
	
	FString CurrentWorldPath;
	
	UPROPERTY()
	TSoftObjectPtr<UStructureSpawnConfigDataAsset> CachedSpawnConfig;
	
	UPROPERTY()
	TObjectPtr<UStructureSpawnService> StructureSpawnService;
};
