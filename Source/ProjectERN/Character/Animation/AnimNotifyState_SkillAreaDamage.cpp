// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Animation/AnimNotifyState_SkillAreaDamage.h"

#include "Abilities/GameplayAbility.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GAS/Abilities/WeaponSkill/ERNGA_WeaponSkill_Instant.h"
#include "GAS/ERNGameplayTags.h"
#include "GameplayAbilitySpec.h"

void UAnimNotifyState_SkillAreaDamage::NotifyBegin(USkeletalMeshComponent* MeshComp,
                                                   UAnimSequenceBase* Animation,
                                                   float TotalDuration,
                                                   const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (!MeshComp)
	{
		return;
	}

	if (UERNGA_WeaponSkill_Instant* Skill = FindActiveInstantWeaponSkill(MeshComp->GetOwner()))
	{
		Skill->BeginAreaDamage(MeshComp);
	}
}

void UAnimNotifyState_SkillAreaDamage::NotifyTick(USkeletalMeshComponent* MeshComp,
                                                  UAnimSequenceBase* Animation,
                                                  float FrameDeltaTime,
                                                  const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);

	if (!MeshComp)
	{
		return;
	}

	if (UERNGA_WeaponSkill_Instant* Skill = FindActiveInstantWeaponSkill(MeshComp->GetOwner()))
	{
		Skill->TickAreaDamage(MeshComp);
	}
}

void UAnimNotifyState_SkillAreaDamage::NotifyEnd(USkeletalMeshComponent* MeshComp,
                                                 UAnimSequenceBase* Animation,
                                                 const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (!MeshComp)
	{
		return;
	}

	if (UERNGA_WeaponSkill_Instant* Skill = FindActiveInstantWeaponSkill(MeshComp->GetOwner()))
	{
		Skill->EndAreaDamage(MeshComp);
	}
}

class UERNGA_WeaponSkill_Instant* UAnimNotifyState_SkillAreaDamage::FindActiveInstantWeaponSkill(
	AActor* OwnerActor) const
{
	if (!OwnerActor)
	{
		return nullptr;
	}

	UAbilitySystemComponent* ASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OwnerActor);

	if (!ASC)
	{
		return nullptr;
	}

	const FGameplayTagContainer WeaponSkillTags(TAG_Ability_Attack_Heavy);

	for (FGameplayAbilitySpec& AbilitySpec : ASC->GetActivatableAbilities())
	{
		if (!AbilitySpec.IsActive() || !AbilitySpec.Ability)
		{
			continue;
		}

		if (!AbilitySpec.Ability->GetAssetTags().HasAll(WeaponSkillTags))
		{
			continue;
		}

		for (UGameplayAbility* AbilityInstance : AbilitySpec.GetAbilityInstances())
		{
			if (UERNGA_WeaponSkill_Instant* InstantSkill =
				Cast<UERNGA_WeaponSkill_Instant>(AbilityInstance))
			{
				return InstantSkill;
			}
		}
	}

	return nullptr;
}
