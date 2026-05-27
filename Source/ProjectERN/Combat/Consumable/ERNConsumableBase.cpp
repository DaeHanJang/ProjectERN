// Fill out your copyright notice in the Description page of Project Settings.

#include "Combat/Consumable/ERNConsumableBase.h"

#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"

// Sets default values
AERNConsumableBase::AERNConsumableBase()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;
	SetReplicates(true);
	
	// Collision
	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	SetRootComponent(Collision);
	Collision->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	Collision->SetNotifyRigidBodyCollision(true);
	Collision->IgnoreActorWhenMoving(GetOwner(), true);
	
	// Mesh
	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(GetRootComponent());
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
	// MovementComponent
	MovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("MovementComponent"));
	MovementComponent->SetUpdatedComponent(GetRootComponent());
	MovementComponent->InitialSpeed = 1200.0f;
	MovementComponent->MaxSpeed = 1200.0f;
	MovementComponent->ProjectileGravityScale = 0.2f;
	MovementComponent->bRotationFollowsVelocity = true;
	
	InitialLifeSpan = 5.0f;
}

void AERNConsumableBase::BeginPlay()
{
	Super::BeginPlay();
	
	Collision->OnComponentHit.AddDynamic(this, &AERNConsumableBase::OnCollisionHit);
}

void AERNConsumableBase::Launch(const FVector& Direction)
{
	if (!MovementComponent || Direction.IsNearlyZero())
	{
		return;
	}
		
	const FVector LaunchDirection = Direction.GetSafeNormal();
	MovementComponent->Velocity = LaunchDirection * MovementComponent->InitialSpeed;
	
	if (MovementComponent->bRotationFollowsVelocity)
	{
		SetActorRotation(LaunchDirection.Rotation());
	}
}

void AERNConsumableBase::OnCollisionHit(UPrimitiveComponent* HitComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (!HasAuthority())
	{
		return;
	}
	
	if (!OtherActor || OtherActor == this || OtherActor == GetOwner() || OtherActor == GetInstigator())
	{
		return;
	}
	
	if (bHit)
	{
		return;
	}
	
	bHit = true;
	ApplyEffect();
	
	UE_LOG(LogTemp, Warning, TEXT("Hit: %s"), *GetNameSafe(OtherActor));
}

void AERNConsumableBase::ApplyEffect()
{
}

void AERNConsumableBase::ApplyEffectPlayer(AProjectERNCharacter* PlayerCharacter)
{
}

void AERNConsumableBase::ApplyEffectMonster(AERNEnemyCharacter* EnemyCharacter)
{
}
