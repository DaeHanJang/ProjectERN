// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ERNPlayerStatusTable.generated.h"

USTRUCT(BlueprintType)
struct FERNPlayerStatusTable : public FTableRowBase
{
	GENERATED_BODY()
	
public:
	virtual void OnDataTableChanged(const UDataTable* InDataTable, const FName InRowName) override;
	
public:
	// 레벨
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Level = 1.0f;
	
	// 비용
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Cost = 0;
	
	// 최대 체력
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxHealth = 100.0f;
	
	// 최대 마나
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxMana = 100.0f;
	
	// 마나 재생
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float ManaRegenRate = 0.01f;
	
	// 최대 스태미나
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float MaxStamina = 100.0f;
	
	// 스태미나 재생
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float StaminaRegenRate = 0.01f;
	
	// 공격력
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float AttackPower = 5.0f;
	
	// 방어력
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Defense = 0.0f;
	
	// 강인도
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float StaggerResistance = 0.0f;
	
	// 강인도(녹 다운)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float DownResistance = 0.0f;
	
};
