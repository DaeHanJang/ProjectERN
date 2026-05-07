// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Abilities/ERNGA_Sprint.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Character/Player/ProjectERNCharacter.h"
#include "GAS/ERNGameplayTags.h"

UERNGA_Sprint::UERNGA_Sprint()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(TAG_Ability_Movement_Sprint);
	SetAssetTags(AssetTags);

	ActivationOwnedTags.AddTag(TAG_State_Movement_Sprinting);
}

void UERNGA_Sprint::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                    const FGameplayAbilityActorInfo* ActorInfo,
                                    const FGameplayAbilityActivationInfo ActivationInfo,
                                    const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (AProjectERNCharacter* Character = Cast<AProjectERNCharacter>(GetAvatarActorFromActorInfo()))
	{
		// 캐릭터 속도/회전모드 갱신
		Character->UpdateMovementSpeed();
		Character->UpdateRotationMode();
	}
	
	// Sprint 시작모션
	if (SprintStartMontage)
	{
		UAbilityTask_PlayMontageAndWait* Task =
			UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
				this,
				NAME_None,
				SprintStartMontage,
				1.0f);

		Task->ReadyForActivation();
	}
}

void UERNGA_Sprint::EndAbility(const FGameplayAbilitySpecHandle Handle,
                               const FGameplayAbilityActorInfo* ActorInfo,
                               const FGameplayAbilityActivationInfo ActivationInfo,
                               bool bReplicateEndAbility,
                               bool bWasCancelled)
{
	AProjectERNCharacter* Character = Cast<AProjectERNCharacter>(GetAvatarActorFromActorInfo());
	
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
	
	if (Character)
	{
		// 캐릭터 속도/회전모드 갱신
		Character->UpdateMovementSpeed();
		Character->UpdateRotationMode();
	}
}

void UERNGA_Sprint::RequestStopSprint()
{
	if (SprintEndMontage)
	{
		// SprintEnd 재생
		// 완료 시 EndAbility
	}
	else
	{
		K2_EndAbility();
	}
}
