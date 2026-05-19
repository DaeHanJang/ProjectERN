// Fill out your copyright notice in the Description page of Project Settings.


#include "Combat/Projectile/ERNCapsuleProjectile.h"

#include "NiagaraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"

AERNCapsuleProjectile::AERNCapsuleProjectile()
{
	CapsuleCollision = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CapsuleCollision"));
	CapsuleCollision->InitCapsuleSize(20.f, 80.f);
	CapsuleCollision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CapsuleCollision->SetCollisionObjectType(ECollisionChannel::ECC_GameTraceChannel1);
	CapsuleCollision->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	CapsuleCollision->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Block);
	CapsuleCollision->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Block);
	CapsuleCollision->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	CapsuleCollision->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel1, ECollisionResponse::ECR_Ignore);

	CapsuleCollision->OnComponentHit.AddDynamic(this, &AERNCapsuleProjectile::OnHit);
	CapsuleCollision->OnComponentBeginOverlap.AddDynamic(this, &AERNCapsuleProjectile::OnBeginOverlap);

	RootComponent = CapsuleCollision;

	if (CollisionComponent)
	{
		CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		CollisionComponent->SetGenerateOverlapEvents(false);
		CollisionComponent->SetupAttachment(CapsuleCollision);
	}

	if (ProjectileMesh)
	{
		ProjectileMesh->SetupAttachment(CapsuleCollision);
	}

	if (TrailEffect)
	{
		TrailEffect->SetupAttachment(CapsuleCollision);
	}

	if (ProjectileMovement)
	{
		ProjectileMovement->SetUpdatedComponent(CapsuleCollision);
	}
}

void AERNCapsuleProjectile::BeginPlay()
{
	Super::BeginPlay();
	
	if (ProjectileMovement && CapsuleCollision)
	{
		ProjectileMovement->SetUpdatedComponent(CapsuleCollision);

		if (ProjectileMovement->Velocity.IsNearlyZero())
		{
			ProjectileMovement->Velocity = GetActorForwardVector() * ProjectileMovement->InitialSpeed;
		}
	}
}
