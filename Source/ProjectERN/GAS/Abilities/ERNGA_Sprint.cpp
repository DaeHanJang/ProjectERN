// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Abilities/ERNGA_Sprint.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Character/Player/ProjectERNCharacter.h"
#include "GAS/ERNAttributeSet.h"
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

	// Stamina 소모 GE 적용
	ApplyStaminaDrainEffect(Handle, ActorInfo, ActivationInfo);

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

	// Stamina 소모 GE 제거
	RemoveStaminaDrainEffect();

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

void UERNGA_Sprint::ApplyStaminaDrainEffect(const FGameplayAbilitySpecHandle Handle,
                                            const FGameplayAbilityActorInfo* ActorInfo,
                                            const FGameplayAbilityActivationInfo ActivationInfo)
{
	if (!ActorInfo || !ActorInfo->AbilitySystemComponent.IsValid() || !SprintStaminaDrainEffectClass)
	{
		return;
	}

	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();

	FGameplayEffectContextHandle ContextHandle = ASC->MakeEffectContext();
	ContextHandle.AddSourceObject(this);

	// GE Spec 만들기
	FGameplayEffectSpecHandle SpecHandle =
		ASC->MakeOutgoingSpec(
			SprintStaminaDrainEffectClass,
			GetAbilityLevel(Handle, ActorInfo),
			ContextHandle);

	if (!SpecHandle.IsValid())
	{
		return;
	}

	// 틱 당 소모될 Cost 값
	const float CostPerTick = StaminaCostPerSecond * StaminaDrainPeriod;

	SpecHandle.Data->SetSetByCallerMagnitude(
		TAG_Data_Cost_Stamina,
		-CostPerTick);

	StaminaDrainEffectHandle = ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());

	ASC->GetGameplayAttributeValueChangeDelegate(UERNAttributeSet::GetStaminaAttribute())
	   .AddUObject(this, &UERNGA_Sprint::HandleStaminaChanged);
}

void UERNGA_Sprint::RemoveStaminaDrainEffect()
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC)
	{
		return;
	}

	if (StaminaDrainEffectHandle.IsValid())
	{
		ASC->RemoveActiveGameplayEffect(StaminaDrainEffectHandle);
		StaminaDrainEffectHandle.Invalidate();
	}

	ASC->GetGameplayAttributeValueChangeDelegate(UERNAttributeSet::GetStaminaAttribute())
		.RemoveAll(this);
}

void UERNGA_Sprint::HandleStaminaChanged(const FOnAttributeChangeData& Data)
{
	if (Data.NewValue <= 0.f)
	{
		K2_EndAbility();
	}
}
