#pragma once

#include "CoreMinimal.h"
#include "MobRuntimeState.generated.h"

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

USTRUCT(BlueprintType)
struct FSpawnerMobRuntimeStates
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FMobRuntimeState> MobStates;
};