// Fill out your copyright notice in the Description page of Project Settings.

#include "Character/Player/AnimNotify_ThrowReceived.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Abilities/GameplayAbilityTypes.h"

void UAnimNotify_ThrowReceived::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	if (!MeshComp || !MeshComp->GetOwner())
	{
		return;
	}
	
	FGameplayEventData EventData;
	EventData.EventTag = EventTag;
	EventData.Instigator = MeshComp->GetOwner();
	
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(MeshComp->GetOwner(), EventTag, EventData);
}
