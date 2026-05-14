// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "MobSpawnPointComponent.generated.h"

class AERNEnemyCharacter;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PROJECTERN_API UMobSpawnPointComponent : public USceneComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UMobSpawnPointComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mob Spawn")
	FName SlotId;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Mob Spawn")
	TSubclassOf<AERNEnemyCharacter> EnemyClass;
};
