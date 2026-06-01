// Fill out your copyright notice in the Description page of Project Settings.

#include "Combat/Consumable/ERNDurationConsumable.h"

#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "NiagaraComponent.h"
#include "Character/Enemy/ERNEnemyCharacter.h"
#include "Character/Player/ProjectERNCharacter.h"
#include "Components/AudioComponent.h"
#include "Components/SphereComponent.h"
#include "Engine/DamageEvents.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/ProjectileMovementComponent.h"

AERNDurationConsumable::AERNDurationConsumable()
{
	// VFX Component
	EffectComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("EffectComponent"));
	EffectComponent->SetupAttachment(GetRootComponent());
	EffectComponent->bAutoActivate = false;
	
	// SFX Component
	SoundComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("SoundComponent"));
	SoundComponent->SetupAttachment(GetRootComponent());
	SoundComponent->bAutoActivate = false;
}

void AERNDurationConsumable::ApplyEffect()
{
	Multicast_PlayEffectAndSound();
	GetWorldTimerManager().SetTimer(ApplyTimerHandle, this, &AERNDurationConsumable::UpdateFogCollision, Rate, true, 0.0f);
}

void AERNDurationConsumable::ApplyEffectPlayer(AProjectERNCharacter* PlayerCharacter)
{
	if (!HasAuthority())
	{
		return;
	}
	if (!GE)
	{
		return;
	}
	
	UAbilitySystemComponent* ASC = PlayerCharacter->GetAbilitySystemComponent();
	if (!ASC)
	{
		return;
	}
	
	ASC->ApplyGameplayEffectToSelf(GE->GetDefaultObject<UGameplayEffect>(), 1.0f, ASC->MakeEffectContext());
}

void AERNDurationConsumable::ApplyEffectMonster(AERNEnemyCharacter* EnemyCharacter)
{
	if (!IsValid(EnemyCharacter))
	{
		return;
	}
	
	EnemyCharacter->TakeDamage(MonsterDamage, FDamageEvent(), GetInstigatorController(), GetOwner());
}

void AERNDurationConsumable::UpdateFogCollision()
{	
	if (!HasAuthority())
	{
		return;
	}
	
	if (CurrentTime >= ApplyTime)
	{
		GetWorldTimerManager().ClearTimer(ApplyTimerHandle);
		Destroy();
		return;
	}
	
	TArray<FOverlapResult> Results;
	
	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);
	
	FCollisionQueryParams QueryParams;
	
	GetWorld()->OverlapMultiByObjectType(Results, GetActorLocation(), FQuat::Identity, ObjectParams, FCollisionShape::MakeSphere(SweepRadius), QueryParams);
	
	TSet<AActor*> AppliedActors;
	
	for (const FOverlapResult& Result : Results)
	{
		AActor* Target = Result.GetActor();
		if (!IsValid(Target) || AppliedActors.Contains(Target))
		{
			continue;
		}
		
		AppliedActors.Add(Target);
		
		if (ApplyType == EApplyType::Player)
		{
			if (AProjectERNCharacter* PlayerCharacter = Cast<AProjectERNCharacter>(Target))
			{
				ApplyEffectPlayer(PlayerCharacter);
			}
		}
		else if (ApplyType == EApplyType::Monster)
		{
			if (AERNEnemyCharacter* EnemyCharacter = Cast<AERNEnemyCharacter>(Target))
			{
				ApplyEffectMonster(EnemyCharacter);
			}
		}
		else if (ApplyType == EApplyType::All)
		{
			if (AProjectERNCharacter* PlayerCharacter = Cast<AProjectERNCharacter>(Target))
			{
				ApplyEffectPlayer(PlayerCharacter);
			}
			if (AERNEnemyCharacter* EnemyCharacter = Cast<AERNEnemyCharacter>(Target))
			{
				ApplyEffectMonster(EnemyCharacter);
			}
		}
	}
	
	CurrentTime += Rate;
}

void AERNDurationConsumable::Multicast_PlayEffectAndSound_Implementation()
{
	Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetHiddenInGame(true);
	MovementComponent->StopMovementImmediately();
	MovementComponent->Deactivate();
	EffectComponent->Activate(true);
	SoundComponent->Play();
}
