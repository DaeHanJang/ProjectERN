// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Abilities/ERNGA_Roll.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GAS/ERNGameplayTags.h"

UERNGA_Roll::UERNGA_Roll()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	AbilityTags.AddTag(TAG_Ability_Move_Roll);

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
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	ACharacter* Character = CastChecked<ACharacter>(ActorInfo->AvatarActor);
	UCharacterMovementComponent* Movement = Character->GetCharacterMovement();

	FVector InputVector = Movement->GetLastInputVector();

	// 1. 섹션 결정
	FName SectionName = GetRollSection(InputVector);

	// 2. 비타겟팅 시에는 입력 방향으로 캐릭터 회전 (타겟팅 시에는 회전하지 않음)
	if (!InputVector.IsNearlyZero())
	{
		Character->SetActorRotation(InputVector.Rotation());
	}

	// 3. 몽타주 섹션 재생
	UAbilityTask_PlayMontageAndWait* Task = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this, NAME_None, RollMontage, 1.0f, SectionName);

	Task->OnCompleted.AddDynamic(this, &UERNGA_Roll::K2_EndAbility);
	Task->OnInterrupted.AddDynamic(this, &UERNGA_Roll::K2_EndAbility);
	Task->OnCancelled.AddDynamic(this, &UERNGA_Roll::K2_EndAbility);
	
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
