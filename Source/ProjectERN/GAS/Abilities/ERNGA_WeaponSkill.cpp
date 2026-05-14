// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Abilities/ERNGA_WeaponSkill.h"

#include "GAS/ERNGameplayTags.h"

UERNGA_WeaponSkill::UERNGA_WeaponSkill()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(TAG_Ability_Attack_Heavy);
	SetAssetTags(AssetTags);
}
