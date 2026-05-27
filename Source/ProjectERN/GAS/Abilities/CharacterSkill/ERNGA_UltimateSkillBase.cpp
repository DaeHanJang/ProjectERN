// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Abilities/CharacterSkill/ERNGA_UltimateSkillBase.h"

#include "GAS/ERNGameplayTags.h"

UERNGA_UltimateSkillBase::UERNGA_UltimateSkillBase()
{
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(TAG_Ability_Skill_Ultimate);
	SetAssetTags(AssetTags);
	
	// 태그 자동 부여
	// 이 베이스를 상속한 모든 궁극기는 궁극기 슬롯 쿨다운을 공유한다.
	CooldownTags.AddTag(TAG_Cooldown_Skill_Ultimate);
}
