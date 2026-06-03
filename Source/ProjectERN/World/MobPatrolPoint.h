// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/TargetPoint.h"
#include "MobPatrolPoint.generated.h"

class AMobSpawner;
/**
 * 
 */
UCLASS()
class PROJECTERN_API AMobPatrolPoint : public ATargetPoint
{
	GENERATED_BODY()
	
public:
	//이 패트롤 포인트의 주인 관계인 몹 스포너
	
	AMobPatrolPoint();
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Patrol")
	FName OwningSpawnerID;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Patrol")
	TObjectPtr<AMobSpawner> OwningSpawner = nullptr;
};
