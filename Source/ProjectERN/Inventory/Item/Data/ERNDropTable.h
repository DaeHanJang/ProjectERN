// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ERNDropTable.generated.h"

USTRUCT(BlueprintType)
struct FERNDropTable : public FTableRowBase
{
	GENERATED_BODY()
	
public:
	// 아이템 키
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ItemID = NAME_None;
	
	// 드랍 확률
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DropChance = 1.0f;
	
	// 최소 드랍 개수 (소모품 전용)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MinCount = 1;
	
	// 최대 드랍 개수 (소모품 전용)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MaxCount = 1;
	
};
