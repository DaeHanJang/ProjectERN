// Copyright Epic Games, Inc. All Rights Reserved.

#include "GAS/Abilities/ERNGA_AttackAbility.h"

#include "Character/Player/ProjectERNCharacter.h"
#include "GAS/ERNGameplayTags.h"

UERNA_AttackAbility::UERNA_AttackAbility()
{
	// 공격 중 태그 자동 부여/제거
	ActivationOwnedTags.AddTag(TAG_State_Combat_Attacking);

	// 공격 중엔 재발동 차단
	ActivationBlockedTags.AddTag(TAG_State_Combat_Attacking);

	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UERNA_AttackAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	
	// 어빌리티 커밋 (코스트/쿨다운 처리)
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}
	
	if (AProjectERNCharacter* Character = Cast<AProjectERNCharacter>(GetAvatarActorFromActorInfo()))
	{
		Character->UpdateMovementSpeed();
	}

	// 몽타주 재생 및 EndAbility는 블루프린트에서 처리
}

void UERNA_AttackAbility::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
	
	if (AProjectERNCharacter* Character = Cast<AProjectERNCharacter>(GetAvatarActorFromActorInfo()))
	{
		Character->UpdateMovementSpeed();
	}
}
