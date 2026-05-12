// Fill out your copyright notice in the Description page of Project Settings.


#include "World/MobSpawner.h"

// Sets default values
AMobSpawner::AMobSpawner()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AMobSpawner::BeginPlay()
{
	Super::BeginPlay();
}

void AMobSpawner::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

// Called every frame
void AMobSpawner::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AMobSpawner::SpawnMobs()
{
	
}

void AMobSpawner::SpawnMobFromPoint(UMobSpawnPointComponent* SpawnPoint)
{
}

void AMobSpawner::RestoreMobFromState(const FMobRuntimeState& State)
{
}

void AMobSpawner::SaveMobStates()
{
}

void AMobSpawner::ClearActiveEnemies()
{
}

