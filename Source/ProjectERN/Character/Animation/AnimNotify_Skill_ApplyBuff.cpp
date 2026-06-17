// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Animation/AnimNotify_Skill_ApplyBuff.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "GAS/ERNGameplayTags.h"

void UAnimNotify_Skill_ApplyBuff::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
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

	FGameplayEventData EventData;
	EventData.EventTag = TAG_Event_Skill_Normal_PaladinShield_Apply;
	EventData.Instigator = OwnerActor;
	EventData.Target = OwnerActor;

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(
		OwnerActor,
		TAG_Event_Skill_Normal_PaladinShield_Apply,
		EventData);
}
