// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Animation/AnimNotify_Ultimate_SpawnAoE.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GAS/ERNGameplayTags.h"
#include "GAS/Abilities/CharacterSkill/ERNGA_Ult_Sanctuary.h"

void UAnimNotify_Ultimate_SpawnAoE::Notify(USkeletalMeshComponent* MeshComp,
                                           UAnimSequenceBase* Animation,
                                           const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!MeshComp)
	{
		return;
	}

	UERNGA_Ult_Sanctuary* SanctuarySkill = FindActiveSanctuarySkill(MeshComp->GetOwner());
	if (!SanctuarySkill)
	{
		return;
	}

	SanctuarySkill->SpawnAoEFromNotify(MeshComp);
}

UERNGA_Ult_Sanctuary* UAnimNotify_Ultimate_SpawnAoE::FindActiveSanctuarySkill(AActor* OwnerActor) const
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

	const FGameplayTagContainer UltimateSkillTags(TAG_Ability_Skill_Ultimate);

	for (FGameplayAbilitySpec& AbilitySpec : ASC->GetActivatableAbilities())
	{
		if (!AbilitySpec.IsActive() || !AbilitySpec.Ability)
		{
			continue;
		}

		if (!AbilitySpec.Ability->GetAssetTags().HasAll(UltimateSkillTags))
		{
			continue;
		}

		for (UGameplayAbility* AbilityInstance : AbilitySpec.GetAbilityInstances())
		{
			UERNGA_Ult_Sanctuary* SanctuarySkill = Cast<UERNGA_Ult_Sanctuary>(AbilityInstance);
			if (SanctuarySkill)
			{
				return SanctuarySkill;
			}
		}
	}

	return nullptr;
}
