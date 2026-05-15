// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Animation/AnimNotify_WeaponSkillBuff.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayAbilitySpec.h"
#include "GAS/ERNGameplayTags.h"
#include "GAS/Abilities/WeaponSkill/ERNGA_WeaponSkill_Buff.h"

void UAnimNotify_WeaponSkillBuff::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
                                         const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);
	
	if (!MeshComp)
	{
		return;
	}

	if (UERNGA_WeaponSkill_Buff* Skill =
		FindActiveBuffWeaponSkill(MeshComp->GetOwner()))
	{
		Skill->ApplyBuffFromNotify(MeshComp);
	}
}

UERNGA_WeaponSkill_Buff* UAnimNotify_WeaponSkillBuff::FindActiveBuffWeaponSkill(AActor* OwnerActor) const
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
			if (UERNGA_WeaponSkill_Buff* BuffSkill = Cast<UERNGA_WeaponSkill_Buff>(AbilityInstance))
			{
				return BuffSkill;
			}
		}
	}

	return nullptr;
}
