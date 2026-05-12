// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Abilities/ERNGA_LightAttack.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Character/Player/ProjectERNCharacter.h"
#include "GAS/ERNGameplayTags.h"

UERNGA_LightAttack::UERNGA_LightAttack()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// Character::TryActivateAbilitiesByTag(TAG_Ability_Attack_Light)로
	// 이 어빌리티를 찾을 수 있게 한다.
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(TAG_Ability_Attack_Light);
	SetAssetTags(AssetTags);
}

void UERNGA_LightAttack::ResetComboState()
{
	// BP에서 몽타주를 재생하기 직전에 호출해주면 된다.
	CurrentComboIndex = 0;
	bHasNextComboInput = false;
	bHasCachedComboRotation = false;
	CachedComboRotation = FRotator::ZeroRotator;
}

void UERNGA_LightAttack::CacheComboInput(const FRotator& TargetRotation)
{
	bHasNextComboInput = true;
	bHasCachedComboRotation = true;
	CachedComboRotation = FRotator(0.f, TargetRotation.Yaw, 0.f);
}

void UERNGA_LightAttack::CheckCombo()
{
	if (!bHasNextComboInput || !CanMoveToNextCombo())
	{
		return;
	}

	const int32 NextComboIndex = CurrentComboIndex + 1;

	const FERNComboSectionCost NextCost =
		ComboSectionCosts.IsValidIndex(NextComboIndex)
			? ComboSectionCosts[NextComboIndex]
			: FERNComboSectionCost();

	if (!ApplyResourceCost(NextCost.StaminaCost, NextCost.ManaCost))
	{
		bHasNextComboInput = false;
		bHasCachedComboRotation = false;
		return;
	}

	bHasNextComboInput = false;
	CurrentComboIndex = NextComboIndex;

	const FName NextSectionName = GetComboSectionName(CurrentComboIndex);
	if (NextSectionName == NAME_None)
	{
		return;
	}
	
	/*
	if (!bHasNextComboInput || !CanMoveToNextCombo())
	{
		return;
	}

	bHasNextComboInput = false;
	++CurrentComboIndex;

	const FName NextSectionName = GetComboSectionName(CurrentComboIndex);
	if (NextSectionName == NAME_None)
	{
		return;
	}
	*/
	
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!AvatarActor)
	{
		return;
	}

	USkeletalMeshComponent* Mesh = AvatarActor->FindComponentByClass<USkeletalMeshComponent>();
	if (!Mesh)
	{
		return;
	}

	UAnimInstance* AnimInstance = Mesh->GetAnimInstance();
	if (!AnimInstance)
	{
		return;
	}

	if (AProjectERNCharacter* ProjectCharacter = Cast<AProjectERNCharacter>(AvatarActor))
	{
		if (bHasCachedComboRotation)
		{
			ProjectCharacter->SetPendingAttackRotation(CachedComboRotation);
		}

		bHasCachedComboRotation = false;
	}
	
	// 몽타주 section 이름은 코드에 직접 쓰지 않고,
	// AttackMontage의 CompositeSections 순서를 콤보 순서로 사용한다.
	AnimInstance->Montage_JumpToSection(NextSectionName, AttackMontage);
}

FName UERNGA_LightAttack::GetComboSectionName(int32 ComboIndex) const
{
	if (!AttackMontage || !AttackMontage->CompositeSections.IsValidIndex(ComboIndex))
	{
		return NAME_None;
	}

	return AttackMontage->CompositeSections[ComboIndex].SectionName;
}

bool UERNGA_LightAttack::CanMoveToNextCombo() const
{
	return AttackMontage &&
		CurrentComboIndex + 1 < AttackMontage->CompositeSections.Num();
}
