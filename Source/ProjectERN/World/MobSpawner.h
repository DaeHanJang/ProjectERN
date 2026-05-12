// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MobSpawner.generated.h"

class UMobSpawnPointComponent;
class AERNEnemyCharacter;

USTRUCT(BlueprintType)
struct FMobRuntimeState
{
	GENERATED_BODY()
	
	UPROPERTY()
	FName SlotId;

	UPROPERTY()
	FTransform Transform;

	UPROPERTY()
	float CurrentHealth = 0.f;

	UPROPERTY()
	bool bDead = false;
};

class AERNEnemyCharacter;

UCLASS()
class PROJECTERN_API AMobSpawner : public AActor
{
	GENERATED_BODY()
	
public:
	// Sets default values for this actor's properties
	AMobSpawner();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	FName GetMobSpawnerID() const { return MobSpawnerID; }
	void SetMobSpawnerID(const FName NewID) { MobSpawnerID = NewID; }
	
private:
	UPROPERTY(EditAnywhere)
	FName MobSpawnerID;
	
	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<AERNEnemyCharacter>> ActiveEnemiesBySlot;
	
	UPROPERTY(EditAnywhere)
	TMap<FName, UMobSpawnPointComponent*> CachedSpawnPoints;
private:
	void SpawnMobs();
	void SpawnMobFromPoint(UMobSpawnPointComponent* SpawnPoint);
	void RestoreMobFromState(const FMobRuntimeState& State);
	void SaveMobStates();
	void ClearActiveEnemies();
};
