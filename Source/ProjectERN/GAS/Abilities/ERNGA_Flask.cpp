// Fill out your copyright notice in the Description page of Project Settings.

#include "GAS/Abilities/ERNGA_Flask.h"

#include "NiagaraFunctionLibrary.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Character/ERNCharacterBase.h"
#include "GAS/ERNAttributeSet.h"
#include "GAS/ERNGameplayTags.h"
#include "Kismet/GameplayStatics.h"

UERNGA_Flask::UERNGA_Flask()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(TAG_Ability_Movement_Flask);
	SetAssetTags(AssetTags);
	
	ActivationOwnedTags.AddTag(TAG_State_Movement_Flask);
	ActivationBlockedTags.AddTag(TAG_State_Movement_Flask);
}

void UERNGA_Flask::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)

{
	if (!GetWorld())
	{
		return;
	}
	
	const AERNCharacterBase* PlayerCharacter = Cast<AERNCharacterBase>(ActorInfo->AvatarActor);
	if (!PlayerCharacter)
	{
		return;
	}
	
	const UERNAttributeSet* AttributeSet = PlayerCharacter->GetAttributeSet();
	if (AttributeSet->GetFlaskQuantity() < 1.0f)
	{
		return;
	}
	
	// GE
	if (GE_Flask)
	{
		ApplyGameplayEffectToOwner(Handle, ActorInfo, ActivationInfo, GE_Flask.GetDefaultObject(), 1.0f);
	}
	
	// Animation
	if (DrinkMontage)
	{
		UAbilityTask_PlayMontageAndWait* Task = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, DrinkMontage);
		Task->OnCompleted.AddDynamic(this, &UERNGA_Flask::K2_EndAbility);
		Task->OnInterrupted.AddDynamic(this, &UERNGA_Flask::K2_EndAbility);
		Task->OnCancelled.AddDynamic(this, &UERNGA_Flask::K2_EndAbility);
		Task->OnBlendOut.AddDynamic(this, &UERNGA_Flask::K2_EndAbility);
	
		Task->ReadyForActivation();
	}
	
	// VFX
	if (Effect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), Effect, PlayerCharacter->GetActorLocation() + FVector(0.0f, 0.0f, -50.0f));
	}
	// SFX
	if (Sound)
	{
		UGameplayStatics::SpawnSoundAtLocation(GetWorld(), Sound, PlayerCharacter->GetActorLocation());
	}
	
	if (!DrinkMontage)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
	}
}
