// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Abilities/ERNGA_Jump.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Character/Player/ProjectERNCharacter.h"
#include "GAS/ERNGameplayTags.h"

UERNGA_Jump::UERNGA_Jump()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	
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
	// 어빌리티 발동 유효성 확인
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	// 캐릭터 유효성 확인
	const AProjectERNCharacter* Character = Cast<AProjectERNCharacter>(ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr);
	if (!Character)
	{
		return false;
	}

	// 캐릭터 유효 && 캐릭터 점프 가능한 상태면 true 반환
	return Character && Character->CanJump();
}

void UERNGA_Jump::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
                                  const FGameplayAbilityActivationInfo ActivationInfo,
                                  const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// 어빌리티 Commit 불가능하면 return
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	
	AProjectERNCharacter* Character = Cast<AProjectERNCharacter>(ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr);
	// 캐릭터 유효성 확인
	if (!Character)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 점프 발동 했는지 확인
	bJumpLaunchConsumed = false;

	// 실제 뛰어 오르기 전 대기 (TAG_Event_Movement_JumpLaunch 기반 실행)
	JumpLaunchEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this,
		TAG_Event_Movement_JumpLaunch,
		nullptr,
		true,
		true);
	
	if (JumpLaunchEventTask)
	{
		JumpLaunchEventTask->EventReceived.AddDynamic(this, &UERNGA_Jump::OnJumpLaunchEvent);
		JumpLaunchEventTask->ReadyForActivation();
	}

	// 몽타주가 유효할 때
	if (JumpMontage)
	{
		// 몽타주 실행 대기
		JumpMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this,
			NAME_None,
			JumpMontage,
			1.f,
			NAME_None,
			true);

		if (JumpMontageTask)
		{
			JumpMontageTask->OnCompleted.AddDynamic(this, &UERNGA_Jump::OnMontageCompleted);
			JumpMontageTask->OnInterrupted.AddDynamic(this, &UERNGA_Jump::OnMontageInterrupted);
			JumpMontageTask->OnCancelled.AddDynamic(this, &UERNGA_Jump::OnMontageCancelled);
			JumpMontageTask->ReadyForActivation();
			return;
		}
	}

	// 실제 캐릭터에서 점프를 담당하는 함수
	Character->ExecuteJumpLaunch();
	FinishJumpAbility(false);
}

void UERNGA_Jump::OnJumpLaunchEvent(FGameplayEventData Payload)
{
	if (bJumpLaunchConsumed)
	{
		return;
	}

	bJumpLaunchConsumed = true;

	AProjectERNCharacter* Character = Cast<AProjectERNCharacter>(GetAvatarActorFromActorInfo());
	if (!Character)
	{
		return;
	}

	// 서버 실행
	if (Character->IsLocallyControlled() || Character->HasAuthority())
	{
		Character->ExecuteJumpLaunch();
	}
}

void UERNGA_Jump::OnMontageCompleted()
{
	// 점프가 소모되지 않은 상태로 어빌리티 종료 시 점프 함수 재실행 
	if (!bJumpLaunchConsumed)
	{
		OnJumpLaunchEvent(FGameplayEventData());
	}

	FinishJumpAbility(false);
}

void UERNGA_Jump::OnMontageInterrupted()
{
	FinishJumpAbility(true);
}

void UERNGA_Jump::OnMontageCancelled()
{
	FinishJumpAbility(true);
}

void UERNGA_Jump::FinishJumpAbility(bool bWasCancelled)
{
	// 어빌리티 종료
	EndAbility(
		GetCurrentAbilitySpecHandle(),
		GetCurrentActorInfo(),
		GetCurrentActivationInfo(),
		true,
		bWasCancelled);
}
