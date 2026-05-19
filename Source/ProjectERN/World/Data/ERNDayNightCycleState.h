#pragma once

#include "CoreMinimal.h"
#include "ERNDayNightCycleState.generated.h"

USTRUCT(BlueprintType)
struct PROJECTERN_API FERNDayNightCycleState
{
	GENERATED_BODY()
	
	UPROPERTY(BlueprintReadOnly)
	bool bRunning = false;
	
	UPROPERTY(BlueprintReadOnly)
	double StartServerWorldTimeSeconds = 0.0;
	
	UPROPERTY(BlueprintReadOnly)
	float Duration = 900.0f;
	
	UPROPERTY(BlueprintReadOnly)
	int32 Revision = 0;
};