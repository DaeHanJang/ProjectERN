// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Animation/AnimNotify_WeaponSkillChanneling.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GAS/ERNGameplayTags.h"
#include "GAS/Abilities/WeaponSkill/ERNGA_WeaponSkill_Channeling.h"

namespace
{
	UERNGA_WeaponSkill_Channeling* FindActiveChannelingWeaponSkill(AActor* OwnerActor)
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
				if (UERNGA_WeaponSkill_Channeling* Skill = Cast<UERNGA_WeaponSkill_Channeling>(AbilityInstance))
				{
					return Skill;
				}
			}
		}

		return nullptr;
	}
}

void UAnimNotify_WeaponSkillChannelingStart::Notify(USkeletalMeshComponent* MeshComp,
                                                    UAnimSequenceBase* Animation,
                                                    const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!MeshComp)
	{
		return;
	}

	if (UERNGA_WeaponSkill_Channeling* Skill =
		FindActiveChannelingWeaponSkill(MeshComp->GetOwner()))
	{
		Skill->StartChannelingFromNotify(MeshComp);
	}
}
