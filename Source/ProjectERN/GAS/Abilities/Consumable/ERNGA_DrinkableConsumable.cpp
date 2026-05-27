// Fill out your copyright notice in the Description page of Project Settings.

#include "GAS/Abilities/Consumable/ERNGA_DrinkableConsumable.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Character/Player/ProjectERNCharacter.h"
#include "GAS/ERNGameplayTags.h"
#include "Inventory/Components/ERNEquipmentComponent.h"

UERNGA_DrinkableConsumable::UERNGA_DrinkableConsumable()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerExecution;
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(TAG_Ability_Movement_Consumable);
	SetAssetTags(AssetTags);
	
	ActivationOwnedTags.AddTag(TAG_State_Movement_Consumable);
	ActivationBlockedTags.AddTag(TAG_State_Movement_Consumable);
}

void UERNGA_DrinkableConsumable::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!GetWorld())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	
	AProjectERNCharacter* PlayerCharacter = Cast<AProjectERNCharacter>(ActorInfo->AvatarActor);
	if (!PlayerCharacter)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	
	if (PlayerCharacter->GetEquipmentComponent()->GetCurrentConsumableQuantity() < 1)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	
	if (HasAuthority(&ActivationInfo))
	{
		PlayerCharacter->GetEquipmentComponent()->Server_UseCurrentConsumableQuantity();
	}
	
	// GE
	if (GE_DrinkableConsumable)
	{
		ApplyGameplayEffectToOwner(Handle, ActorInfo, ActivationInfo, GE_DrinkableConsumable.GetDefaultObject(), 1.0f);
	}
	
	// Animation
	if (DrinkMontage)
	{
		UAbilityTask_PlayMontageAndWait* Task = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, DrinkMontage);
		Task->OnCompleted.AddDynamic(this, &UERNGA_DrinkableConsumable::K2_EndAbility);
		Task->OnInterrupted.AddDynamic(this, &UERNGA_DrinkableConsumable::K2_EndAbility);
		Task->OnCancelled.AddDynamic(this, &UERNGA_DrinkableConsumable::K2_EndAbility);
		Task->OnBlendOut.AddDynamic(this, &UERNGA_DrinkableConsumable::K2_EndAbility);
	
		Task->ReadyForActivation();
	}
	
	// VFX, SFX
	PlayerCharacter->Multicast_PlayEffectAndSound(Effect, PlayerCharacter->GetActorLocation() + FVector(0.0f, 0.0f, -50.0f), Sound, PlayerCharacter->GetActorLocation());
	
	if (!DrinkMontage)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
	}
}
