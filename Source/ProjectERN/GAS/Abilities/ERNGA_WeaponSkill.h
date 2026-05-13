// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GAS/Abilities/ERNGA_AttackAbility.h"
#include "ERNGA_WeaponSkill.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTERN_API UERNGA_WeaponSkill : public UERNA_AttackAbility
{
	GENERATED_BODY()
	
public:
	UERNGA_WeaponSkill();
	
	// 몽타주는 부모 클래스에 있는 UERNA_AttackAbility 적용
};
