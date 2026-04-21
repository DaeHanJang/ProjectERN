// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ERNItemEnums.h"
#include "Inventory/Item/Data/ItemDataAssetBase.h"
#include "ConsumableItemDataAsset.generated.h"

class UGameplayAbility;

/**
 * 
 */
UCLASS()
class PROJECTERN_API UConsumableItemDataAsset : public UItemDataAssetBase
{
	GENERATED_BODY()
	
public:
	// 사용 가능 여부
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EConsumableType ConsumableType = EConsumableType::None; 
	
	// 사용 소모품 효과
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(EditCondition="ConsumableType == EConsumableType::Usable"))
	TSoftClassPtr<UGameplayAbility> ConsumableAbility = nullptr;
	
};
