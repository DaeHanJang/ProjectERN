// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ERNItemEnums.h"
#include "Inventory/Item/Data/ItemDataAssetBase.h"
#include "EquipableItemDataAsset.generated.h"

class UGameplayAbility;

/**
 * 
 */
UCLASS()
class PROJECTERN_API UEquipableItemDataAsset : public UItemDataAssetBase
{
	GENERATED_BODY()
	
public:
	virtual void GatherSoftPaths(const EItemAssetLoadFlags LoadFlags, TArray<FSoftObjectPath>& OutPaths) const override;
	
public:
	// 무기 종류
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EWeaponType WeaponType = EWeaponType::None;
	
	// 약공격 대미지
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", UIMin="0"))
	int32 LightAttackDamage = 0;
	
	// 강공격 대미지
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", UIMin="0"))
	int32 HeavyAttackDamage = 0;
	
	// 약공격 강인도
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 LightAttackStaggerPower = 0;
	
	// 강공격 강인도
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 HeavyAttackStaggerPower = 0;
	
	// 무기 고유 스킬
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftClassPtr<UGameplayAbility> WeaponSkillAbility = nullptr;
		
};
