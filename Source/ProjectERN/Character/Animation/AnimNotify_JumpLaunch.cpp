// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Animation/AnimNotify_JumpLaunch.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "GAS/ERNGameplayTags.h"

void UAnimNotify_JumpLaunch::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
                                    const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!MeshComp)
	{
		return;
	}

	AActor* OwnerActor = MeshComp->GetOwner();
	if (!OwnerActor)
	{
		return;
	}

	// Ability Tag 대신 Event Tag로 발동하도록 수정
	FGameplayEventData EventData;
	// EventData.EventTag = TAG_Ability_Movement_Jump;
	EventData.EventTag = TAG_Event_Movement_JumpLaunch;
	EventData.Instigator = OwnerActor;
	EventData.Target = OwnerActor;
	
	// UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(OwnerActor, TAG_Ability_Movement_Jump, EventData);
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(OwnerActor, TAG_Event_Movement_JumpLaunch, EventData);
}

FString UAnimNotify_JumpLaunch::GetNotifyName_Implementation() const
{
	return TEXT("Jump Launch");
}
