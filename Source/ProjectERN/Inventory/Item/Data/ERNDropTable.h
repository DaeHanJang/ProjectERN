// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ERNDropTable.generated.h"

UENUM(BlueprintType)
enum class EDropItemType : uint8
{
	None       UMETA(DisplayName="None"),
	Sword      UMETA(DisplayName="Sword"),
	Staff      UMETA(DisplayName="Staff"),
	Polearm    UMETA(DisplayName="Polearm"),
	Consumable UMETA(DisplayName="Consumable")
};

USTRUCT(BlueprintType)
struct FERNDropTable : public FTableRowBase
{
	GENERATED_BODY()
	
public:
	// 아이템 키
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName ItemID = NAME_None;
	
	// 아이템 종류
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EDropItemType Type = EDropItemType::None;
	
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
