// Fill out your copyright notice in the Description page of Project Settings.


#include "Actors/AoE/ERNAoE_Heal.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Character/Player/ProjectERNCharacter.h"
#include "GAS/ERNGameplayTags.h"

bool AERNAoE_Heal::IsValidAoETarget(AActor* TargetActor, UPrimitiveComponent* OverlappedComponent) const
{
	if (!Super::IsValidAoETarget(TargetActor, OverlappedComponent))
	{
		return false;
	}

	// Sanctuary 힐 장판은 죽지 않은 현재 플레이어 캐릭터만 회복 대상으로 본다.
	AProjectERNCharacter* TargetCharacter = Cast<AProjectERNCharacter>(TargetActor);
	if (!TargetCharacter || !TargetCharacter->IsAlive())
	{
		return false;
	}

	// RootComponent가 Overlap됐을 때만 적용
	if (OverlappedComponent != TargetCharacter->GetRootComponent())
	{
		return false;
	}
	
	if (!HealEffectClass || HealAmountPerTick <= 0.f)
	{
		return false;
	}
	
	if (!GetASCFromActor(TargetCharacter))
	{
		return false;
	}
	
	return true;
}

void AERNAoE_Heal::ApplyAoEToTarget(AActor* TargetActor)
{
	AProjectERNCharacter* TargetCharacter = Cast<AProjectERNCharacter>(TargetActor);
	if (!TargetCharacter)
	{
		return;
	}

	UAbilitySystemComponent* SourceASC = GetASCFromActor(SourceActor);
	UAbilitySystemComponent* TargetASC = GetASCFromActor(TargetCharacter);
	if (!SourceASC || !TargetASC || !HealEffectClass)
	{
		return;
	}

	FGameplayEffectContextHandle ContextHandle = SourceASC->MakeEffectContext();
	ContextHandle.AddSourceObject(this);
	ContextHandle.AddInstigator(SourceActor, this);

	FGameplayEffectSpecHandle SpecHandle = SourceASC->MakeOutgoingSpec(HealEffectClass, 1.f, ContextHandle);

	if (!SpecHandle.IsValid())
	{
		return;
	}

	SpecHandle.Data->SetSetByCallerMagnitude(TAG_Data_Heal_Amount, HealAmountPerTick);

	SourceASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
}

UAbilitySystemComponent* AERNAoE_Heal::GetASCFromActor(AActor* Actor) const
{
	if (!Actor)
	{
		return nullptr;
	}

	IAbilitySystemInterface* AbilitySystemInterface = Cast<IAbilitySystemInterface>(Actor);
	if (!AbilitySystemInterface)
	{
		return nullptr;
	}

	return AbilitySystemInterface->GetAbilitySystemComponent();
}
