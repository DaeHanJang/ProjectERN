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
	CapsuleCollision->SetGenerateOverlapEvents(true);
	CapsuleCollision->SetCollisionObjectType(ECC_GameTraceChannel1);
	CapsuleCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	CapsuleCollision->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	CapsuleCollision->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	CapsuleCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	CapsuleCollision->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Ignore);

	CapsuleCollision->OnComponentHit.AddDynamic(this, &AERNCapsuleProjectile::OnHit);
	CapsuleCollision->OnComponentBeginOverlap.AddDynamic(this, &AERNCapsuleProjectile::OnPierceOverlap);

	SetRootComponent(CapsuleCollision);

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

UPrimitiveComponent* AERNCapsuleProjectile::GetProjectileCollisionComponent() const
{
	// CapsuleCollision이 있다면 교체
	return CapsuleCollision ? CapsuleCollision : Super::GetProjectileCollisionComponent();
}

