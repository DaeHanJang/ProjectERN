// Fill out your copyright notice in the Description page of Project Settings.


#include "Combat/Projectile/ERNBoxProjectile.h"

#include "NiagaraComponent.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"

AERNBoxProjectile::AERNBoxProjectile()
{
	BoxCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxCollision"));
	BoxCollision->InitBoxExtent(FVector(80.f, 20.f, 20.f));
	BoxCollision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	BoxCollision->SetGenerateOverlapEvents(true);
	BoxCollision->SetCollisionObjectType(ECC_GameTraceChannel1);
	BoxCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	BoxCollision->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	BoxCollision->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	BoxCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	BoxCollision->OnComponentHit.AddDynamic(this, &AERNBoxProjectile::OnHit);
	BoxCollision->OnComponentBeginOverlap.AddDynamic(this, &AERNBoxProjectile::OnPierceOverlap);
	
	SetRootComponent(BoxCollision);

	if (CollisionComponent)
	{
		CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		CollisionComponent->SetGenerateOverlapEvents(false);
		CollisionComponent->SetupAttachment(BoxCollision);
	}

	if (ProjectileMesh)
	{
		ProjectileMesh->SetupAttachment(BoxCollision);
	}

	if (TrailEffect)
	{
		TrailEffect->SetupAttachment(BoxCollision);
	}

	if (ProjectileMovement)
	{
		ProjectileMovement->SetUpdatedComponent(BoxCollision);
	}
}

void AERNBoxProjectile::BeginPlay()
{
	Super::BeginPlay();
	
	if (ProjectileMovement && BoxCollision)
	{
		ProjectileMovement->SetUpdatedComponent(BoxCollision);

		if (ProjectileMovement->Velocity.IsNearlyZero())
		{
			ProjectileMovement->Velocity = GetActorForwardVector() * ProjectileMovement->InitialSpeed;
		}
	}
}

UPrimitiveComponent* AERNBoxProjectile::GetProjectileCollisionComponent() const
{
	// BoxCollision이 있다면 교체
	return BoxCollision ? BoxCollision : Super::GetProjectileCollisionComponent();
}
