// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Abilities/ERNGA_Roll.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Character/Player/ProjectERNCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GAS/ERNGameplayTags.h"

UERNGA_Roll::UERNGA_Roll()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(TAG_Ability_Movement_Roll);
	SetAssetTags(AssetTags);

	ActivationOwnedTags.AddTag(TAG_State_Combat_Rolling);
	ActivationBlockedTags.AddTag(TAG_State_Combat_Rolling);
}

void UERNGA_Roll::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                  const FGameplayAbilityActorInfo* ActorInfo,
                                  const FGameplayAbilityActivationInfo ActivationInfo,
                                  const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		// 중복 종료 방어
		if (!IsActive())
		{
			return;
		}
		
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	AProjectERNCharacter* Character = CastChecked<AProjectERNCharacter>(ActorInfo->AvatarActor);

	// const FVector InputVector = Movement->GetLastInputVector();	// 구르기 시 입력 방향 받아옴
	const FVector InputVector = Character->GetPendingRollDirection();	// 멀티 환경에서 적용하기 위해 수정
	const bool bIsLockOn = Character->IsLockOn();

	// 기본 구르기 섹션
	FName SectionName = FName("Roll_F");

	// 락온 상태에서만 방향 결정 가능
	if (bIsLockOn)
	{
		SectionName = GetRollSection(InputVector);
	}

	// 비타겟팅 시에는 입력 방향으로 캐릭터 회전 (타겟팅 시에는 회전하지 않음)
	if (!bIsLockOn && !InputVector.IsNearlyZero())
	{
		Character->SetActorRotation(InputVector.Rotation());
	}

	// 몽타주 섹션 재생
	UAbilityTask_PlayMontageAndWait* Task = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, RollMontage, 1.0f, SectionName);

	Task->OnCompleted.AddDynamic(this, &UERNGA_Roll::K2_EndAbility);
	Task->OnInterrupted.AddDynamic(this, &UERNGA_Roll::K2_EndAbility);
	Task->OnCancelled.AddDynamic(this, &UERNGA_Roll::K2_EndAbility);
	Task->OnBlendOut.AddDynamic(this, &UERNGA_Roll::K2_EndAbility);
	
	Task->ReadyForActivation();
}

FName UERNGA_Roll::GetRollSection(const FVector& InputVector)
{
	// 1. 입력이 없거나 시점 고정이 아니면 기본 'Forward' 섹션
	if (InputVector.IsNearlyZero())
	{
		return FName("Roll_F");
	}

	ACharacter* Character = CastChecked<ACharacter>(GetAvatarActorFromActorInfo());

	// 2. 캐릭터의 전방 방향을 기준으로 입력 벡터의 상대 각도 계산
	FVector Forward = Character->GetActorForwardVector();
	FVector Right = Character->GetActorRightVector();

	float ForwardDot = FVector::DotProduct(Forward, InputVector.GetSafeNormal());
	float RightDot = FVector::DotProduct(Right, InputVector.GetSafeNormal());

	// 아크탄젠트를 이용해 -180 ~ 180도 사이의 각도 산출
	float Angle = FMath::RadiansToDegrees(FMath::Atan2(RightDot, ForwardDot));

	// 3. 각도에 따른 8방향 섹션 매핑
	if (Angle >= -22.5f && Angle < 22.5f)   return FName("Roll_F");
	if (Angle >= 22.5f && Angle < 67.5f)    return FName("Roll_FR");
	if (Angle >= 67.5f && Angle < 112.5f)   return FName("Roll_R");
	if (Angle >= 112.5f && Angle < 157.5f)  return FName("Roll_BR");
	if (Angle >= 157.5f || Angle < -157.5f) return FName("Roll_B");
	if (Angle >= -157.5f && Angle < -112.5f) return FName("Roll_BL");
	if (Angle >= -112.5f && Angle < -67.5f)  return FName("Roll_L");
	if (Angle >= -67.5f && Angle < -22.5f)   return FName("Roll_FL");

	return FName("Roll_F");
}
