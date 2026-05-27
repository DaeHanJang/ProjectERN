// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Animation/AnimNotify_Ultimate_Explosion.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GAS/ERNGameplayTags.h"
#include "GAS/Abilities/CharacterSkill/ERNGA_UltimateSkillBase.h"

void UAnimNotify_Ultimate_Explosion::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
                                            const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);
	
	if (!MeshComp)
	{
		return;
	}

	// 폭발 적용
	if (UERNGA_UltimateSkillBase* UltimateSkill = FindActiveUltimateSkill(MeshComp->GetOwner()))
	{
		UltimateSkill->TriggerExplosionFromNotify(MeshComp);
	}
}

UERNGA_UltimateSkillBase* UAnimNotify_Ultimate_Explosion::FindActiveUltimateSkill(AActor* OwnerActor) const
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

	// 발동 태그
	const FGameplayTagContainer UltimateSkillTags(TAG_Ability_Skill_Ultimate);

	// 현재 캐릭터의 Ability 등록 정보(AbilitySpec)에서, 지금 실행 중인 궁극기 Ability 인스턴스 탐색
	for (FGameplayAbilitySpec& AbilitySpec : ASC->GetActivatableAbilities())
	{
		// 지금 실행 중인 Ability가 아니면 || Ability 정보가 비어 있다면 Skip
		if (!AbilitySpec.IsActive() || !AbilitySpec.Ability)
		{
			continue;
		}

		// UltimateSkillTags 태그를 가진 Ability인지 확인
		if (!AbilitySpec.Ability->GetAssetTags().HasAll(UltimateSkillTags))
		{
			continue;
		}

		// 실제 인스턴스 탐색
		for (UGameplayAbility* AbilityInstance : AbilitySpec.GetAbilityInstances())
		{
			// 실제 인스턴스가 UERNGA_UltimateSkillBase 계열인지 확인
			if (UERNGA_UltimateSkillBase* UltimateSkill = Cast<UERNGA_UltimateSkillBase>(AbilityInstance))
			{
				// 인스턴스 return
				return UltimateSkill;
			}
		}
	}

	return nullptr;
}
