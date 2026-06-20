// Copyright Epic Games, Inc. All Rights Reserved.

#include "Combat/Projectile/ERNEldenStar.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Character/ERNCharacterBase.h"
#include "Character/Enemy/ERNEnemyCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "TimerManager.h"

AERNEldenStar::AERNEldenStar()
{
	// 폰과 충돌 무시
	if (CollisionComponent)
	{
		CollisionComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
	}

	// 머리 위 유도용 좌표
	HomingProxy = CreateDefaultSubobject<USceneComponent>(TEXT("HomingProxy"));
	HomingProxy->SetupAttachment(RootComponent);
	HomingProxy->SetUsingAbsoluteLocation(true);

	// 속도 기본값
	if (ProjectileMovement)
	{
		ProjectileMovement->InitialSpeed = 400.f;
		ProjectileMovement->MaxSpeed = 400.f;
	}

	// 유도/폭발 기본 활성화
	bHomingEnabled = true;
	HomingAcceleration = 1500.f;
	bExplode = true;
	ExplosionRadius = 400.f;
	ExplosionDamage = 60.f;
	ExplosionStaggerPower = 40.f;

	// HomingProxy 위치 갱신용 Tick
	PrimaryActorTick.bStartWithTickEnabled = true;

	// Duration은 자체 타이머로 관리
	InitialLifeSpan = 0.f;
}

void AERNEldenStar::BeginPlay()
{
	Super::BeginPlay();

	// 베이스의 InitializeHoming이 PMC->HomingTargetComponent를 타겟 RootComponent로 설정함
	// EldenStar는 HomingProxy를 추적해야 하므로 덮어씀
	if (ProjectileMovement && HomingProxy)
	{
		ProjectileMovement->bIsHomingProjectile = true;
		ProjectileMovement->HomingTargetComponent = HomingProxy;
		ProjectileMovement->HomingAccelerationMagnitude = HomingAcceleration;
	}

	if (!HasAuthority()) return;

	// 첫 타겟
	EnsureValidTarget();

	// 타이머 시작
	UWorld* World = GetWorld();
	if (!World) return;

	World->GetTimerManager().SetTimer(
		RetargetTimerHandle, this, &AERNEldenStar::OnRetargetTick,
		RetargetCheckInterval, true);

	World->GetTimerManager().SetTimer(
		FireTimerHandle, this, &AERNEldenStar::OnFireTick,
		FireInterval, true, InitialFireDelay);

	World->GetTimerManager().SetTimer(
		DurationTimerHandle, this, &AERNEldenStar::OnDurationEnded,
		Duration, false);
}

void AERNEldenStar::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(RetargetTimerHandle);
		World->GetTimerManager().ClearTimer(FireTimerHandle);
		World->GetTimerManager().ClearTimer(DurationTimerHandle);
	}

	Super::EndPlay(EndPlayReason);
}

void AERNEldenStar::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 타겟 머리 위로 위치 갱신 (서버/클라 양쪽)
	AActor* Target = HomingTarget.Get();
	if (Target && HomingProxy)
	{
		const FVector Loc = Target->GetActorLocation() + FVector(0.f, 0.f, HomingZOffset);
		HomingProxy->SetWorldLocation(Loc);
	}

	// OnRep_HomingTarget이 PMC HomingTargetComponent를 Target RootComponent로 덮어쓸 수 있음 - 매 틱 보정
	if (ProjectileMovement && HomingProxy
		&& ProjectileMovement->HomingTargetComponent != HomingProxy)
	{
		ProjectileMovement->HomingTargetComponent = HomingProxy;
	}
}

void AERNEldenStar::OnRetargetTick()
{
	if (!HasAuthority()) return;
	EnsureValidTarget();
}

void AERNEldenStar::EnsureValidTarget()
{
	AActor* Current = HomingTarget.Get();
	AERNCharacterBase* CurrentCharBase = Cast<AERNCharacterBase>(Current);
	const bool bAlive = (Current != nullptr) && (!CurrentCharBase || !CurrentCharBase->IsDead());

	if (bAlive) return;

	AActor* NewTarget = FindNearestEnemy();
	if (NewTarget)
	{
		HomingTarget = NewTarget;	// 리플리케이트
	}
	else
	{
		HomingTarget.Reset();
	}
}

AActor* AERNEldenStar::FindNearestEnemy() const
{
	TArray<AActor*> Enemies;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AERNEnemyCharacter::StaticClass(), Enemies);

	AActor* Best = nullptr;
	float BestDistSq = RetargetSearchRadius * RetargetSearchRadius;
	const FVector Origin = GetActorLocation();

	for (AActor* E : Enemies)
	{
		if (!IsValid(E)) continue;

		AERNEnemyCharacter* Enemy = Cast<AERNEnemyCharacter>(E);
		if (!Enemy || Enemy->IsDead()) continue;

		const float DistSq = FVector::DistSquared(Origin, E->GetActorLocation());
		if (DistSq > BestDistSq) continue;

		BestDistSq = DistSq;
		Best = E;
	}
	return Best;
}

AActor* AERNEldenStar::FindRandomEnemyInRange() const
{
	TArray<AActor*> Enemies;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AERNEnemyCharacter::StaticClass(), Enemies);

	TArray<AActor*> InRange;
	const float RadiusSq = ChildTargetSearchRadius * ChildTargetSearchRadius;
	const FVector Origin = GetActorLocation();

	for (AActor* E : Enemies)
	{
		if (!IsValid(E)) continue;

		AERNEnemyCharacter* Enemy = Cast<AERNEnemyCharacter>(E);
		if (!Enemy || Enemy->IsDead()) continue;

		if (FVector::DistSquared(Origin, E->GetActorLocation()) > RadiusSq) continue;

		InRange.Add(E);
	}

	if (InRange.Num() == 0)
	{
		return nullptr;
	}
	return InRange[FMath::RandRange(0, InRange.Num() - 1)];
}

void AERNEldenStar::OnFireTick()
{
	if (!HasAuthority() || !SubProjectileClass) return;

	FActorSpawnParameters Params;
	Params.Owner = GetOwner();
	Params.Instigator = GetInstigator();
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	const FVector SpawnLoc = GetActorLocation();
	const FRotator SpawnRot = GetActorRotation();

	AERNProjectileBase* Sub = GetWorld()->SpawnActor<AERNProjectileBase>(
		SubProjectileClass, SpawnLoc, SpawnRot, Params);

	// 자식은 주변 범위의 적 중 랜덤 1마리로 유도 (여러 적 분산 타격). 후보 없으면 부모 타겟으로 폴백
	AActor* ChildTarget = FindRandomEnemyInRange();
	if (!ChildTarget)
	{
		ChildTarget = HomingTarget.Get();
	}

	if (Sub && ChildTarget)
	{
		Sub->HomingTarget = ChildTarget;
		Sub->ApplyHomingSettings(ChildTarget);
	}

	Multicast_PlayFireEffect(SpawnLoc);
}

void AERNEldenStar::Multicast_PlayFireEffect_Implementation(FVector Location)
{
	if (FireEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(), FireEffect, Location, FRotator::ZeroRotator);
	}
	if (FireSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this, FireSound, Location, FRotator::ZeroRotator,
			1.f, 1.f, 0.f, FireSoundAttenuation);
	}
}

void AERNEldenStar::OnDurationEnded()
{
	if (!HasAuthority()) return;

	const FVector Loc = GetActorLocation();

	// Duration 만료 - 자기 위치에서 범위 폭발 (베이스 변수 사용)
	if (bExplode)
	{
		ApplyExplosionDamage(Loc);
	}

	Multicast_PlayImpactEffect(Loc, GetActorRotation());
	SetLifeSpan(0.2f);	// 클라이언트 이펙트 재생 시간 확보
}
