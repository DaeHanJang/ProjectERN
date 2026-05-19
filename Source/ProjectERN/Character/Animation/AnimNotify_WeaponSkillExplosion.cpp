// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Animation/AnimNotify_WeaponSkillExplosion.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GAS/ERNGameplayTags.h"
#include "GAS/Abilities/WeaponSkill/ERNGA_WeaponSkill_Instant.h"

void UAnimNotify_WeaponSkillExplosion::Notify(USkeletalMeshComponent* MeshComp,
                                              UAnimSequenceBase* Animation,
                                              const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);
	
	if (!MeshComp)
	{
		return;
	}

	if (UERNGA_WeaponSkill_Instant* Skill = FindActiveInstantWeaponSkill(MeshComp->GetOwner()))
	{
		Skill->ExplodeFromNotify(MeshComp);
	}
}

UERNGA_WeaponSkill_Instant* UAnimNotify_WeaponSkillExplosion::FindActiveInstantWeaponSkill(
	AActor* OwnerActor) const
{
	if (!OwnerActor)
	{
		return nullptr;
	}

	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OwnerActor);

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
