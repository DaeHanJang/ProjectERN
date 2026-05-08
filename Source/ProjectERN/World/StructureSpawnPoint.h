// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Data/EStructureType.h"
#include "Engine/TargetPoint.h"
#include "StructureSpawnPoint.generated.h"



/**
 * 
 */
UCLASS()
class PROJECTERN_API AStructureSpawnPoint : public ATargetPoint
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Structure Spawn Type")
	EStructureType StructureType;
	
};
