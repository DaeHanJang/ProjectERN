// Fill out your copyright notice in the Description page of Project Settings.

#include "GAS/Abilities/Consumable/ERNGA_ThrowableConsumable.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Character/Player/ProjectERNCharacter.h"
#include "Combat/Consumable/ERNConsumableBase.h"
#include "GAS/ERNGameplayTags.h"
#include "Inventory/Components/ERNEquipmentComponent.h"
#include "Inventory/Item/Data/ConsumableItemDataAsset.h"
#include "Inventory/Item/Manager/ItemManagerSubsystem.h"

UERNGA_ThrowableConsumable::UERNGA_ThrowableConsumable()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerExecution;
	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(TAG_Ability_Movement_Consumable);
	SetAssetTags(AssetTags);
	
	ActivationOwnedTags.AddTag(TAG_State_Movement_Consumable);
	ActivationBlockedTags.AddTag(TAG_State_Movement_Consumable);
}

void UERNGA_ThrowableConsumable::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, 
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!GetWorld())
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}
	
	UAbilityTask_WaitGameplayEvent* WaitEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, FGameplayTag::RequestGameplayTag(TEXT("Event.Consumable.Throw")));
	WaitEventTask->EventReceived.AddDynamic(this, &UERNGA_ThrowableConsumable::OnThrowNotifyReceived);
	
	WaitEventTask->ReadyForActivation();
	
	if (ThrowMontage)
	{
		UAbilityTask_PlayMontageAndWait* AnimTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, ThrowMontage);
		AnimTask->OnCompleted.AddDynamic(this, &UERNGA_ThrowableConsumable::K2_EndAbility);
		AnimTask->OnInterrupted.AddDynamic(this, &UERNGA_ThrowableConsumable::K2_EndAbility);
		AnimTask->OnCancelled.AddDynamic(this, &UERNGA_ThrowableConsumable::K2_EndAbility);
		AnimTask->OnBlendOut.AddDynamic(this, &UERNGA_ThrowableConsumable::K2_EndAbility);
	
		AnimTask->ReadyForActivation();
	}
	
	if (!ThrowMontage)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
	}
}

void UERNGA_ThrowableConsumable::OnThrowNotifyReceived(FGameplayEventData Payload)
{
	if (!GetWorld() || !GetWorld()->GetGameInstance())
	{
		return;
	}
	
	AProjectERNCharacter* PlayerCharacter = Cast<AProjectERNCharacter>(GetAvatarActorFromActorInfo());
	if (!PlayerCharacter || !PlayerCharacter->HasAuthority())
	{
		return;
	}
	
	UItemManagerSubsystem* ItemManager = GetWorld()->GetGameInstance()->GetSubsystem<UItemManagerSubsystem>();
	if (!ItemManager)
	{
		return;
	}
	
	const FName ItemID = PlayerCharacter->GetEquipmentComponent()->GetCurrentConsumableItemID();
	if (!ItemManager->ItemValid(ItemID))
	{
		return;
	}
	
	const UConsumableItemDataAsset* DA = Cast<const UConsumableItemDataAsset>(ItemManager->LoadItemDataAssetSync(ItemID, EItemAssetLoadFlags::Gameplay));
	if (!DA)
	{
		return;
	}
	if (DA->ConsumableClass.IsNull())
	{
		return;
	}
	
	const FVector SpawnLocation = PlayerCharacter->GetActorLocation() + PlayerCharacter->GetActorForwardVector() * ThrowForwardDistance;
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = PlayerCharacter;
	GetWorld()->SpawnActor<AERNConsumableBase>(DA->ConsumableClass.Get(), SpawnLocation, FRotator::ZeroRotator, SpawnParams);
}
