// Fill out your copyright
// notice in the Description page of Project Settings.

#include "Combat/Projectile/ERNPiercingProjectile.h"

#include "NiagaraComponent.h"
#include "Character/Enemy/ERNEnemyCharacter.h"
#include "Character/Player/ProjectERNCharacter.h"
#include "Components/SphereComponent.h"
#include "Engine/DamageEvents.h"
#include "GameFramework/ProjectileMovementComponent.h"

AERNPiercingProjectile::AERNPiercingProjectile()
{
	if (CollisionComponent)
	{
		CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		CollisionComponent->SetGenerateOverlapEvents(true);
		CollisionComponent->SetCollisionObjectType(ECC_GameTraceChannel1);
		CollisionComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
		CollisionComponent->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
		CollisionComponent->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
		CollisionComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
		CollisionComponent->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Ignore);

		CollisionComponent->OnComponentBeginOverlap.RemoveDynamic(this, &AERNPiercingProjectile::OnBeginOverlap);
		CollisionComponent->OnComponentBeginOverlap.AddDynamic(this, &AERNPiercingProjectile::OnPierceOverlap);
	}
}

void AERNPiercingProjectile::BeginPlay()
{
	Super::BeginPlay();

	if (ProjectileMovement && ProjectileMovement->Velocity.IsNearlyZero())
	{
		// 속도 적용
		ProjectileMovement->Velocity = GetActorForwardVector() * ProjectileMovement->InitialSpeed;
	}
}

void AERNPiercingProjectile::DisableProjectileCollision()
{
	UPrimitiveComponent* ProjectileCollision = GetProjectileCollisionComponent();

	if (ProjectileCollision)
	{
		ProjectileCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		ProjectileCollision->SetGenerateOverlapEvents(false);
	}

	if (CollisionComponent && CollisionComponent != ProjectileCollision)
	{
		CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		CollisionComponent->SetGenerateOverlapEvents(false);
	}

	SetActorEnableCollision(false);
}

void AERNPiercingProjectile::OnPierceOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
                                             UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// 수명이 끝났다면 종료
	if (bPierceFinished)
	{
		return;
	}
	
	// 서버에서만 실행
	if (!HasAuthority())
	{
		return;
	}

	// 시전자 제외
	if (!OtherActor || OtherActor == GetOwner())
	{
		return;
	}

	// 이미 관통된 대상이면 제외
	if (PiercedActors.Contains(OtherActor))
	{
		return;
	}

	AActor* OwnerActor = GetOwner();
	
	AProjectERNCharacter* PlayerShooter = Cast<AProjectERNCharacter>(OwnerActor);
	AERNEnemyCharacter* EnemyShooter = Cast<AERNEnemyCharacter>(OwnerActor);

	AProjectERNCharacter* OtherPlayer = Cast<AProjectERNCharacter>(OtherActor);
	AERNEnemyCharacter* OtherEnemy = Cast<AERNEnemyCharacter>(OtherActor);

	// 아군/적 구분
	if (PlayerShooter && OtherPlayer) return;
	if (EnemyShooter && OtherEnemy) return;

	if (PlayerShooter && !OtherEnemy) return;
	if (EnemyShooter && !OtherPlayer) return;
	if (!PlayerShooter && !EnemyShooter) return;

	// Impact Effect 적용 지점
	const FVector ImpactPoint = bFromSweep ? FVector(SweepResult.ImpactPoint) : OtherActor->GetActorLocation();

	// Impact Effect의 회전값
	const FRotator ImpactRot = bFromSweep ? SweepResult.ImpactNormal.Rotation() : GetActorRotation();

	// 대미지/경직 부여
	if (OtherEnemy)
	{
		OtherEnemy->TakeDamage(Damage, FDamageEvent(), GetInstigatorController(), OwnerActor);
		// 투사체 충돌 지점을 HitOrigin으로 전달 → 적이 투사체 방향 기준 4방향 경직
		OtherEnemy->TryApplyStagger(StaggerPower, ImpactPoint);
	}
	else if (OtherPlayer)
	{
		OtherPlayer->TakeDamage(Damage, FDamageEvent(), GetInstigatorController(), OwnerActor);
		OtherPlayer->TryApplyStagger(StaggerPower, ImpactPoint);
	}

	// 관통 시 대상 배열에 추가
	PiercedActors.Add(OtherActor);
	// 관통 횟수 증가
	CurrentPierceCount++;

	// 폭발 적용
	if (bExplode)
	{
		ApplyExplosionDamage(ImpactPoint);
	}

	// 피격 대상에 ImpactEffect 적용
	if (bPlayImpactEffectOnEachHit)
	{
		Multicast_PlayImpactEffect(ImpactPoint, ImpactRot);
	}

	// 조건 만족 시 투사체 삭제(무제한 관통이 아님 && 최대 관통 수 도달)
	if (!bUnlimitedPierce && CurrentPierceCount >= MaxPierceCount)
	{
		FinishPiercingProjectile();
	}
}

void AERNPiercingProjectile::FinishPiercingProjectile()
{
	// 이미 수명 다 됐다면 종료 (중복 실행 방지)
	if (bPierceFinished)
	{
		return;
	}
	
	// 종료 적용
	bPierceFinished = true;

	// 움직임 삭제
	if (ProjectileMovement)
	{
		ProjectileMovement->StopMovementImmediately();
		ProjectileMovement->Deactivate();
	}

	// 충돌 판정 제거
	DisableProjectileCollision();
	// 외형 숨김 처리
	SetActorHiddenInGame(true);

	// Effect 비활성화
	if (TrailEffect)
	{
		TrailEffect->Deactivate();
	}

	// 0.2초후 제거 (안정적인 Effect 출력을 위함)
	SetLifeSpan(0.2f);
}