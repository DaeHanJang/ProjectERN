// Fill out your copyright notice in the Description page of Project Settings.

#include "Combat/Consumable/ERNInstantConsumable.h"

#include "AbilitySystemComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Character/Enemy/ERNEnemyCharacter.h"
#include "Character/Player/ProjectERNCharacter.h"
#include "Engine/DamageEvents.h"
#include "Engine/OverlapResult.h"
#include "Kismet/GameplayStatics.h"

void AERNInstantConsumable::ApplyEffect()
{
	if (!GetWorld())
	{
		return;
	}
	
	TArray<FOverlapResult> OverlapResults;
	FCollisionShape Sphere = FCollisionShape::MakeSphere(SweepRadius);

	if (GetWorld()->OverlapMultiByChannel(OverlapResults, GetActorLocation(), FQuat::Identity, ECollisionChannel::ECC_Pawn, Sphere))
	{
		for (const FOverlapResult& OverlapResult : OverlapResults)
		{
			AActor* OverlapActor = OverlapResult.GetActor();
			if (!OverlapActor)
			{
				continue;
			}
			
			if (ApplyType == EApplyType::Player)
			{
				if (AProjectERNCharacter* PlayerCharacter = Cast<AProjectERNCharacter>(OverlapActor))
				{
					ApplyEffectPlayer(PlayerCharacter);
				}
			}
			else if (ApplyType == EApplyType::Monster)
			{
				if (AERNEnemyCharacter* EnemyCharacter = Cast<AERNEnemyCharacter>(OverlapActor))
				{
					ApplyEffectMonster(EnemyCharacter);
				}
			}
			else if (ApplyType == EApplyType::All)
			{
				if (AProjectERNCharacter* PlayerCharacter = Cast<AProjectERNCharacter>(OverlapActor))
				{
					ApplyEffectPlayer(PlayerCharacter);
				}
				else if (AERNEnemyCharacter* EnemyCharacter = Cast<AERNEnemyCharacter>(OverlapActor))
				{
					ApplyEffectMonster(EnemyCharacter);
				}
			}
		}
	}
	
	if (Effect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), Effect, GetActorLocation());
	}
	
	if (Sound)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), Sound, GetActorLocation());
	}
}

void AERNInstantConsumable::ApplyEffectPlayer(AProjectERNCharacter* PlayerCharacter)
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

void AERNInstantConsumable::ApplyEffectMonster(AERNEnemyCharacter* EnemyCharacter)
{
	if (!HasAuthority())
	{
		return;
	}
	
	if (MonsterEffectType == EMonsterEffectType::Instant)
	{
		EnemyCharacter->TakeDamage(MonsterDamage, FDamageEvent(), GetInstigatorController(), GetOwner());
	}
	else if (MonsterEffectType == EMonsterEffectType::Duration)
	{
		MonsterApplyTimerDelegates.Add(FTimerDelegate());
		MonsterApplyTimers.Add(FTimerHandle());
		const int32 TimerIndex = CurrentTimers.Add(0.0f);
		MonsterApplyTimerDelegates.Top().BindUObject(this, &AERNInstantConsumable::UpdateApplyDamageToMonster, EnemyCharacter, TimerIndex);
		GetWorldTimerManager().SetTimer(MonsterApplyTimers.Top(), MonsterApplyTimerDelegates.Top(), MonsterRate, true, 0.0f);
	}
}

void AERNInstantConsumable::UpdateApplyDamageToMonster(AERNEnemyCharacter* EnemyCharacter, int32 Index)
{
	EnemyCharacter->TakeDamage(MonsterDamage, FDamageEvent(), GetInstigatorController(), GetOwner());
	
	if (CurrentTimers[Index] >= MonsterApplyTime)
	{
		GetWorldTimerManager().ClearTimer(MonsterApplyTimers[Index]);
		return;
	}
	
	CurrentTimers[Index] += MonsterRate;
}
