// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ERNItemRuntimeState.generated.h"

// Item Runtime State
USTRUCT(BlueprintType)
struct FItemRuntimeState
{
	GENERATED_BODY()
	
public:
	FORCEINLINE bool IsValid() const { return ItemID != NAME_None; }
		
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	FName ItemID = NAME_None;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	int32 Quantity = 1;
	
};
