// Fill out your copyright notice in the Description page of Project Settings.


#include "MiddleBossSpawner.h"

#include "EngineUtils.h"
#include "NightRainZoneManager.h"
#include "Character/Enemy/ERNEnemyCharacter.h"


// Sets default values
AMiddleBossSpawner::AMiddleBossSpawner()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
}

void AMiddleBossSpawner::SpawnMob()
{
	if (EnemyClass == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("AMiddleBossSpawner:: EnemyClass is nullptr"));
		return;
	}
	
	FActorSpawnParameters SpawnParams;
	AERNEnemyCharacter* SpawnedEnemy = GetWorld()->SpawnActor<AERNEnemyCharacter>(EnemyClass, GetActorTransform(),SpawnParams);
	
	if (SpawnedEnemy == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("SpawnedEnemy is nullptr failed spawn enemy"));
		return; 
	}
}
