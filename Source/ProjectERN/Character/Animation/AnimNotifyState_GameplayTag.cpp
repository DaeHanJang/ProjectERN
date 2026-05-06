// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/Animation/AnimNotifyState_GameplayTag.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"

// NotifyState에서 태그를 활용하기 위한 헬퍼함수
static UAbilitySystemComponent* GetASCFromMesh(USkeletalMeshComponent* MeshComp)
{
	if (!MeshComp)
	{
		return nullptr;
	}

	AActor* Owner = MeshComp->GetOwner();
	if (!Owner)
	{
		return nullptr;
	}

	if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Owner))
	{
		return ASI->GetAbilitySystemComponent();
	}

	return nullptr;
}

void UAnimNotifyState_GameplayTag::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
                                               float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);
	
	if (UAbilitySystemComponent* ASC = GetASCFromMesh(MeshComp))
	{
		ASC->AddLooseGameplayTag(Tag);
	}
}

void UAnimNotifyState_GameplayTag::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);
	if (UAbilitySystemComponent* ASC = GetASCFromMesh(MeshComp))
	{
		ASC->RemoveLooseGameplayTag(Tag);
	}
}
