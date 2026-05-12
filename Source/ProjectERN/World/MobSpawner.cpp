// Fill out your copyright notice in the Description page of Project Settings.


#include "World/MobSpawner.h"

#include "ERNWorldManagerSubsystem.h"
#include "MobSpawnPointComponent.h"

// Sets default values
AMobSpawner::AMobSpawner()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

}

// Called when the game starts or when spawned
void AMobSpawner::BeginPlay()
{
	Super::BeginPlay();
	
	if (HasAuthority() == false)
	{
		return;
	}
	
	if (MobSpawnerID.IsNone() == true)
	{
		UE_LOG(LogTemp, Warning, TEXT("MobSpawner ID is None"));
		return;
	}
	
	// 부착된 UMobSpawnPointComponent 들 수집
	CachedSpawnPoints.Empty();
	
	TArray<UMobSpawnPointComponent*> MobSpawnPoints;
	GetComponents<UMobSpawnPointComponent>(MobSpawnPoints);
	
	for (UMobSpawnPointComponent* SpawnPoint : MobSpawnPoints)
	{
		if (SpawnPoint == nullptr)
		{
			continue;
		}
		
		if (SpawnPoint->SlotId.IsNone())
		{
			continue;
		}
		
		if (SpawnPoint->EnemyClass.IsNull())
		{
			continue;
		}
		
		CachedSpawnPoints.Add(SpawnPoint->SlotId, SpawnPoint);
	}
	
	// 스포너에 저장된 데이터를 월드 매니저에서 가져오기
	UERNWorldManagerSubsystem* WorldManager = GetWorld()->GetSubsystem<UERNWorldManagerSubsystem>();
	if (WorldManager == nullptr)
	{
		// ERN 월드 매니저는 없어도 스폰은 가능하도록 구현
		SpawnMobs();
		return;
	}
	
	
	
	// 저장된 데이터가 없다면 SpawnMobs
	
	// 있다면 SpawnMobFromPoint
	
}

void AMobSpawner::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (HasAuthority() == false)
	{
		return;
	}
	
	if (EndPlayReason == EEndPlayReason::RemovedFromWorld)
	{
		
	}
	
	SaveMobStates();
	
	ClearActiveEnemies();
	
	Super::EndPlay(EndPlayReason);
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
	if (ActiveEnemiesBySlot.IsEmpty() == true)
	{
		return;
	}
	
	
}

