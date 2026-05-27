// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Abilities/CharacterSkill/ERNGA_NormalSkillBase.h"

#include "GAS/ERNGameplayTags.h"

UERNGA_NormalSkillBase::UERNGA_NormalSkillBase()
{
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(TAG_Ability_Skill_Normal);
	SetAssetTags(AssetTags);
	
	// 태그 자동 부여
	// 이 베이스를 상속한 모든 일반 스킬은 일반 스킬 슬롯 쿨다운을 공유한다.
	CooldownTags.AddTag(TAG_Cooldown_Skill_Normal);
}
