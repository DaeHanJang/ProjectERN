// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MiddleBossSpawner.generated.h"

class AERNEnemyCharacter;

UCLASS()
class PROJECTERN_API AMiddleBossSpawner : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AMiddleBossSpawner();

	UFUNCTION()
	void SpawnMob();

private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy", meta = (AllowPrivateAccess = true))
	TSubclassOf<AERNEnemyCharacter> EnemyClass;
};
