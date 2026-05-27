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
#include "GameFramework/ProjectileMovementComponent.h"

AERNDurationConsumable::AERNDurationConsumable()
{
	// Fog Collision
	FogCollision = CreateDefaultSubobject<USphereComponent>(TEXT("FogCollision"));
	FogCollision->SetupAttachment(GetRootComponent());
	FogCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FogCollision->SetGenerateOverlapEvents(true);
	
	// VFX Component
	EffectComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("EffectComponent"));
	EffectComponent->SetupAttachment(GetRootComponent());
	EffectComponent->bAutoActivate = false;
	
	// SFX Component
	SoundComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("SoundComponent"));
	SoundComponent->SetupAttachment(GetRootComponent());
	SoundComponent->bAutoActivate = false;
}

void AERNDurationConsumable::BeginPlay()
{
	Super::BeginPlay();
	
	FogCollision->InitSphereRadius(SweepRadius);
	// FogCollision Overlap Event Binding
	FogCollision->OnComponentBeginOverlap.AddDynamic(this, &AERNDurationConsumable::OnFogBeginOverlap);
	FogCollision->OnComponentEndOverlap.AddDynamic(this, &AERNDurationConsumable::OnFogEndOverlap);
}

void AERNDurationConsumable::ApplyEffect()
{
	FogCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
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
	EnemyCharacter->TakeDamage(MonsterDamage, FDamageEvent(), GetInstigatorController(), GetOwner());
}

void AERNDurationConsumable::UpdateFogCollision()
{	
	FogCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CurrentTime += Rate;
	
	if (CurrentTime >= ApplyTime)
	{
		GetWorldTimerManager().ClearTimer(ApplyTimerHandle);
		Destroy();
	}
}

void AERNDurationConsumable::OnFogBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                               UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!OtherActor || OtherActor == this)
	{
		return;
	}
	
	if (OverlappedActors.Contains(OtherActor))
	{
		return;
	}
	
	OverlappedActors.Add(OtherActor);
}

void AERNDurationConsumable::OnFogEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!OtherActor || OtherActor == this)
	{
		return;
	}
	
	if (OverlappedActors.Remove(OtherActor) == 0)
	{
		return;
	}
	
	if (ApplyType == EApplyType::Player)
	{
		if (AProjectERNCharacter* PlayerCharacter = Cast<AProjectERNCharacter>(OtherActor))
		{
			ApplyEffectPlayer(PlayerCharacter);
		}
	}
	else if (ApplyType == EApplyType::Monster)
	{
		if (AERNEnemyCharacter* EnemyCharacter = Cast<AERNEnemyCharacter>(OtherActor))
		{
			ApplyEffectMonster(EnemyCharacter);
		}
	}
	else if (ApplyType == EApplyType::All)
	{
		if (AProjectERNCharacter* PlayerCharacter = Cast<AProjectERNCharacter>(OtherActor))
		{
			ApplyEffectPlayer(PlayerCharacter);
		}
		if (AERNEnemyCharacter* EnemyCharacter = Cast<AERNEnemyCharacter>(OtherActor))
		{
			ApplyEffectMonster(EnemyCharacter);
		}
	}
	
	FogCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
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
