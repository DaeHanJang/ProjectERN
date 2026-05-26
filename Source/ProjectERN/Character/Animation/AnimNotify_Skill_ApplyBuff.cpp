// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Animation/AnimNotify_Skill_ApplyBuff.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "GAS/Abilities/CharacterSkill/ERNGA_Normal_PaladinShield.h"

void UAnimNotify_Skill_ApplyBuff::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
                                         const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);
	
	if (!MeshComp)
	{
		return;
	}

	if (UERNGA_Normal_PaladinShield* Skill = FindActivePaladinShieldSkill(MeshComp->GetOwner()))
	{
		Skill->ApplyShieldFromNotify(MeshComp);
	}
}

UERNGA_Normal_PaladinShield* UAnimNotify_Skill_ApplyBuff::FindActivePaladinShieldSkill(AActor* OwnerActor) const
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

	for (FGameplayAbilitySpec& AbilitySpec : ASC->GetActivatableAbilities())
	{
		if (!AbilitySpec.IsActive())
		{
			continue;
		}

		for (UGameplayAbility* AbilityInstance : AbilitySpec.GetAbilityInstances())
		{
			if (UERNGA_Normal_PaladinShield* PaladinShieldSkill = Cast<UERNGA_Normal_PaladinShield>(AbilityInstance))
			{
				return PaladinShieldSkill;
			}
		}
	}

	return nullptr;
}
