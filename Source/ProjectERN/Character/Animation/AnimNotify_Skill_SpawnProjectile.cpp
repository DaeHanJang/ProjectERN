// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Animation/AnimNotify_Skill_SpawnProjectile.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GAS/Abilities/CharacterSkill/ERNGA_Normal_StarFall.h"

void UAnimNotify_Skill_SpawnProjectile::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
                                               const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);
	
	if (!MeshComp)
	{
		return;
	}

	if (UERNGA_Normal_StarFall* Skill = FindActiveStarFallSkill(MeshComp->GetOwner()))
	{
		Skill->FireProjectileFromNotify(MeshComp);
	}
}

TObjectPtr<UERNGA_Normal_StarFall> UAnimNotify_Skill_SpawnProjectile::FindActiveStarFallSkill(AActor* OwnerActor) const
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
			if (UERNGA_Normal_StarFall* StarFallSkill =
				Cast<UERNGA_Normal_StarFall>(AbilityInstance))
			{
				return StarFallSkill;
			}
		}
	}

	return nullptr;
}
