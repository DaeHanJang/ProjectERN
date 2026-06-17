// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Animation/AnimNotify_Skill_SpawnProjectile.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GAS/Abilities/CharacterSkill/ERNGA_Normal_StarFall.h"
#include "GAS/Abilities/CharacterSkill/ERNGA_Ult_Mage.h"

namespace
{
	// 오너의 활성 어빌리티 중 지정 타입의 인스턴스를 찾아 반환
	template<typename TAbility>
	TAbility* FindActiveAbilityOfType(AActor* OwnerActor)
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
				if (TAbility* Typed = Cast<TAbility>(AbilityInstance))
				{
					return Typed;
				}
			}
		}

		return nullptr;
	}
}

void UAnimNotify_Skill_SpawnProjectile::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
                                               const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!MeshComp)
	{
		return;
	}

	AActor* OwnerActor = MeshComp->GetOwner();

	// 활성화된 투사체 발사 스킬을 찾아 발사 (StarFall / 메이지 궁극기 등)
	if (UERNGA_Normal_StarFall* StarFall = FindActiveAbilityOfType<UERNGA_Normal_StarFall>(OwnerActor))
	{
		StarFall->FireProjectileFromNotify(MeshComp, SpawnLocationOffset);
		return;
	}
	if (UERNGA_Ult_Mage* UltMage = FindActiveAbilityOfType<UERNGA_Ult_Mage>(OwnerActor))
	{
		UltMage->FireProjectileFromNotify(MeshComp, SpawnLocationOffset);
	}
}
