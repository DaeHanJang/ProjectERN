// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ERNItemEnums.h"
#include "Inventory/Item/Data/ItemDataAssetBase.h"
#include "ConsumableItemDataAsset.generated.h"

class AERNConsumableBase;
class UGameplayAbility;

/**
 * 
 */
UCLASS()
class PROJECTERN_API UConsumableItemDataAsset : public UItemDataAssetBase
{
	GENERATED_BODY()
	
public:
	virtual void GatherSoftPaths(const EItemAssetLoadFlags LoadFlags, TArray<FSoftObjectPath>& OutPaths) const override;
	
public:
	// 사용 가능 여부
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EConsumableType ConsumableType = EConsumableType::None;
	
	// 사용 타입
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(EditCondition="ConsumableType == EConsumableType::Usable"))
	EUsableType UsableType = EUsableType::None;
	
	// 사용 소모품 효과
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(EditCondition="ConsumableType == EConsumableType::Usable"))
	TSoftClassPtr<UGameplayAbility> ConsumableAbility = nullptr;
	
	// 소모품 클래스
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(EditCondition="ConsumableType == EConsumableType::Usable && UsableType == EUsableType::Throwable"))
	TSoftClassPtr<AERNConsumableBase> ConsumableClass = nullptr;
	
};
