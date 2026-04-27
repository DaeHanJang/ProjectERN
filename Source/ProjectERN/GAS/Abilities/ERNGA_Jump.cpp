// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Abilities/ERNGA_Jump.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Character/Player/ProjectERNCharacter.h"
#include "GAS/ERNGameplayTags.h"

UERNGA_Jump::UERNGA_Jump()
{
	ActivationOwnedTags.AddTag(TAG_State_Combat_Jumping);
	
	ActivationBlockedTags.AddTag(TAG_State_Combat_Jumping);
	
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
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
