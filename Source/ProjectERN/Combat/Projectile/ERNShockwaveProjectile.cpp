// Copyright Epic Games, Inc. All Rights Reserved.

#include "Combat/Projectile/ERNShockwaveProjectile.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/DecalComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Character/Player/ProjectERNCharacter.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "Engine/DamageEvents.h"
#include "DrawDebugHelpers.h"

AERNShockwaveProjectile::AERNShockwaveProjectile()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	// 직접 충돌하지 않음 - 반경 오버랩을 수동으로 처리
	if (CollisionComponent)
	{
		CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	// 제자리 고정 (이동/중력 없음)
	if (ProjectileMovement)
	{
		ProjectileMovement->InitialSpeed = 0.f;
		ProjectileMovement->MaxSpeed = 0.f;
		ProjectileMovement->ProjectileGravityScale = 0.f;
	}

	// 링/디스크 메시
	RingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RingMesh"));
	RingMesh->SetupAttachment(RootComponent);
	RingMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 충격파 Niagara
	ShockwaveNiagara = CreateDefaultSubobject<UNiagaraComponent>(TEXT("ShockwaveNiagara"));
	ShockwaveNiagara->SetupAttachment(RootComponent);
	ShockwaveNiagara->SetAutoActivate(false);

	// 지면 데칼 (아래로 투영)
	RingDecal = CreateDefaultSubobject<UDecalComponent>(TEXT("RingDecal"));
	RingDecal->SetupAttachment(RootComponent);
	RingDecal->SetRelativeRotation(FRotator(-90.f, 0.f, 0.f));

	// 자체 타이머/수명으로 관리
	InitialLifeSpan = 0.f;
}

void AERNShockwaveProjectile::BeginPlay()
{
	Super::BeginPlay();

	// 지면 스냅 (서버/클라 동일하게 결정적 트레이스 → 원점 Z 일치)
	if (bSnapToGround)
	{
		const FVector Loc = GetActorLocation();
		FHitResult Hit;
		const FVector TraceStart = Loc + FVector(0.f, 0.f, GroundTraceHalfHeight);
		const FVector TraceEnd = Loc - FVector(0.f, 0.f, GroundTraceHalfHeight);

		FCollisionQueryParams Params;
		Params.AddIgnoredActor(this);
		if (GetOwner()) Params.AddIgnoredActor(GetOwner());

		if (GetWorld()->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, Params))
		{
			SetActorLocation(FVector(Loc.X, Loc.Y, Hit.ImpactPoint.Z));
		}
	}

	// VFX 초기화 (할당된 것만)
	if (ShockwaveFX && ShockwaveNiagara)
	{
		ShockwaveNiagara->SetAsset(ShockwaveFX);
		ShockwaveNiagara->Activate(true);
	}

	if (RingDecalMaterial && RingDecal)
	{
		RingDecal->SetDecalMaterial(RingDecalMaterial);
		RingDecal->DecalSize = FVector(DecalProjectionDepth, MaxRadius, MaxRadius);
		DecalMID = RingDecal->CreateDynamicMaterialInstance();
	}

	UpdateVisual();
}

void AERNShockwaveProjectile::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 시간 기반 반경 (서버/클라 동일하게 결정적)
	CurrentRadius = FMath::Min(GetGameTimeSinceCreation() * ExpandSpeed, MaxRadius);

	UpdateVisual();

	if (bDrawDebug)
	{
		DrawDebugCircle(
			GetWorld(), GetActorLocation(), CurrentRadius, DebugSegments,
			DebugColor, false, -1.f, 0, 3.f,
			FVector(0, 1, 0), FVector(1, 0, 0), false);
	}

	if (HasAuthority())
	{
		SweepDamage();

		// 최대 반경 도달 → 잠깐 남겼다가 소멸
		if (CurrentRadius >= MaxRadius)
		{
			SetLifeSpan(0.3f);
		}
	}
}

void AERNShockwaveProjectile::UpdateVisual()
{
	// 1) Niagara (외곽 반경 + 띠 두께 + 최대 반경 → 링 띠 그리기)
	if (ShockwaveNiagara && ShockwaveFX)
	{
		ShockwaveNiagara->SetVariableFloat(RadiusParamName, CurrentRadius);
		ShockwaveNiagara->SetVariableFloat(ThicknessParamName, FrontThickness);
		ShockwaveNiagara->SetVariableFloat(MaxRadiusParamName, MaxRadius);
	}

	// 2) 링 메시 스케일 (외곽 반경에 맞춤)
	if (RingMesh && RingMesh->GetStaticMesh())
	{
		const float Scale = (RingMeshUnitRadius > 0.f) ? (CurrentRadius / RingMeshUnitRadius) : 1.f;
		RingMesh->SetRelativeScale3D(FVector(Scale, Scale, Scale));
	}

	// 3) 데칼 머티리얼 (외곽 반경 + 띠 두께 + 최대 반경 → 링 띠 그리기)
	if (DecalMID)
	{
		DecalMID->SetScalarParameterValue(RadiusParamName, CurrentRadius);
		DecalMID->SetScalarParameterValue(ThicknessParamName, FrontThickness);
		DecalMID->SetScalarParameterValue(MaxRadiusParamName, MaxRadius);
	}
}

void AERNShockwaveProjectile::SweepDamage()
{
	UWorld* World = GetWorld();
	if (!World) return;

	const FVector Origin = GetActorLocation();

	// 현재 반경 구체 오버랩 (Pawn만)
	TArray<FOverlapResult> Overlaps;
	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_Pawn);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ERNShockwaveSweep), false);
	QueryParams.AddIgnoredActor(this);
	if (GetOwner()) QueryParams.AddIgnoredActor(GetOwner());

	const bool bHit = World->OverlapMultiByObjectType(
		Overlaps, Origin, FQuat::Identity, ObjectParams,
		FCollisionShape::MakeSphere(CurrentRadius), QueryParams);

	if (bHit)
	{
		for (const FOverlapResult& Result : Overlaps)
		{
			AProjectERNCharacter* Player = Cast<AProjectERNCharacter>(Result.GetActor());
			if (!Player || Player->IsDead()) continue;
			if (HitActors.Contains(Player)) continue;

			const FVector PlayerLoc = Player->GetActorLocation();

			// front(앞면) 띠 안에 있는 대상만 (수평거리 기준)
			const float D = FVector::Dist2D(Origin, PlayerLoc);
			if (D > CurrentRadius) continue;					// 아직 front 도달 전
			if (D < CurrentRadius - FrontThickness) continue;	// front가 이미 지나감

			// 점프로 떠 있으면 회피
			if (PlayerLoc.Z - Origin.Z > MaxHitHeight) continue;

			// 타격
			Player->TakeDamage(Damage, FDamageEvent(), GetInstigatorController(), GetOwner());
			Player->TryApplyStagger(StaggerPower, Origin);

			if (bKnockbackEnabled)
			{
				ApplyKnockback(Player, PlayerLoc - Origin, KnockbackForce);
			}

			HitActors.Add(Player);
		}
	}
}
