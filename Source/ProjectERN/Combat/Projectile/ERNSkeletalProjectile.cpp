// Copyright Epic Games, Inc. All Rights Reserved.

#include "Combat/Projectile/ERNSkeletalProjectile.h"
#include "Components/SkeletalMeshComponent.h"

AERNSkeletalProjectile::AERNSkeletalProjectile()
{
	SkeletalMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMesh"));
	SkeletalMesh->SetupAttachment(RootComponent);

	// 비주얼 전용 - 충돌/오버랩은 베이스의 CollisionComponent가 담당하므로 끈다.
	SkeletalMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SkeletalMesh->SetGenerateOverlapEvents(false);
}
