// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Abilities/ERNGA_Jump.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Character/Player/ProjectERNCharacter.h"
#include "GAS/ERNGameplayTags.h"

UERNGA_Jump::UERNGA_Jump()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(TAG_Ability_Movement_Jump);
	SetAssetTags(AssetTags);
	
	// 점프 중 태그 자동 부여/제거
	ActivationOwnedTags.AddTag(TAG_State_Combat_Jumping);
	
	// 재발동 차단
	ActivationBlockedTags.AddTag(TAG_State_Combat_Jumping);
}

bool UERNGA_Jump::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                     const FGameplayAbilityActorInfo* ActorInfo,
                                     const FGameplayTagContainer* SourceTags,
                                     const FGameplayTagContainer* TargetTags,
                                     FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	const AProjectERNCharacter* Character = Cast<AProjectERNCharacter>(ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr);
	if (!Character)
	{
		return false;
	}

	return Character->CanJump();
}

void UERNGA_Jump::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
                                  const FGameplayAbilityActivationInfo ActivationInfo,
                                  const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}
}
