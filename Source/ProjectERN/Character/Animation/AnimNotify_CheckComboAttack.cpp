// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Animation/AnimNotify_CheckComboAttack.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameplayAbilitySpec.h"
#include "GAS/Abilities/ERNGA_LightAttack.h"
#include "GAS/ERNGameplayTags.h"

void UAnimNotify_CheckComboAttack::Notify(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!MeshComp)
	{
		return;
	}

	AActor* Owner = MeshComp->GetOwner();
	if (!Owner)
	{
		return;
	}

	IAbilitySystemInterface* AbilitySystemInterface = Cast<IAbilitySystemInterface>(Owner);
	if (!AbilitySystemInterface)
	{
		return;
	}

	UAbilitySystemComponent* ASC = AbilitySystemInterface->GetAbilitySystemComponent();
	if (!ASC)
	{
		return;
	}

	const FGameplayTagContainer LightAttackTags(TAG_Ability_Attack_Light);
	for (const FGameplayAbilitySpec& AbilitySpec : ASC->GetActivatableAbilities())
	{
		if (!AbilitySpec.IsActive() || !AbilitySpec.Ability ||
			!AbilitySpec.Ability->GetAssetTags().HasAll(LightAttackTags))
		{
			continue;
		}

		for (UGameplayAbility* AbilityInstance : AbilitySpec.GetAbilityInstances())
		{
			if (UERNGA_LightAttack* LightAttackAbility = Cast<UERNGA_LightAttack>(AbilityInstance))
			{
				LightAttackAbility->CheckCombo();
				return;
			}
		}
	}
}

FString UAnimNotify_CheckComboAttack::GetNotifyName_Implementation() const
{
	return TEXT("Check Combo Attack");
}
