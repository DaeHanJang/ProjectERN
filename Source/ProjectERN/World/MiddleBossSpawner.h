// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MiddleBossSpawner.generated.h"

class AERNEnemyCharacter;
class ANightRainZoneManager;

UCLASS()
class PROJECTERN_API AMiddleBossSpawner : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AMiddleBossSpawner();
	

	
protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	void BindNightRainManager();
	
	UFUNCTION()
	void SpawnMob(float ZonePhase);
	
public:
	// Called every frame
	virtual void Tick(float DeltaTime) override;
	
private:
	UPROPERTY()
	TObjectPtr<ANightRainZoneManager> BoundNightRainManager;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Enemy", meta = (AllowPrivateAccess = true))
	TSubclassOf<AERNEnemyCharacter> EnemyClass;
};
