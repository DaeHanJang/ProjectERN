// Copyright Epic Games, Inc. All Rights Reserved.

#include "Combat/Projectile/ERNProjectileBase.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Engine/DamageEvents.h"
#include "Kismet/GameplayStatics.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Net/UnrealNetwork.h"
#include "Curves/CurveFloat.h"
#include "Character/Enemy/ERNEnemyCharacter.h"
#include "Character/Player/ProjectERNCharacter.h"

AERNProjectileBase::AERNProjectileBase()
{
	// Tick은 가속 활성화 시에만 BeginPlay에서 켬
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	bReplicates = true;

	CollisionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionComponent"));
	CollisionComponent->InitSphereRadius(15.0f);
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	// 커스텀 Projectile 채널 (DefaultEngine.ini의 GameTraceChannel1)
	CollisionComponent->SetCollisionObjectType(ECollisionChannel::ECC_GameTraceChannel1);
	CollisionComponent->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	CollisionComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldStatic, ECollisionResponse::ECR_Block);
	CollisionComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_WorldDynamic, ECollisionResponse::ECR_Block);
	// 폰은 Overlap 처리 - 같은 팀 통과를 위해 OnBeginOverlap에서 타입 분기
	CollisionComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
	// 같은 Projectile 채널끼리는 통과 (투사체끼리 충돌 방지)
	CollisionComponent->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel1, ECollisionResponse::ECR_Ignore);
	CollisionComponent->OnComponentHit.AddDynamic(this, &AERNProjectileBase::OnHit);
	CollisionComponent->OnComponentBeginOverlap.AddDynamic(this, &AERNProjectileBase::OnBeginOverlap);
	RootComponent = CollisionComponent;

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->InitialSpeed = 1200.0f;
	ProjectileMovement->MaxSpeed = 1200.0f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->ProjectileGravityScale = 0.0f;

	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProjectileMesh"));
	ProjectileMesh->SetupAttachment(RootComponent);
	ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	TrailEffect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("TrailEffect"));
	TrailEffect->SetupAttachment(RootComponent);

	InitialLifeSpan = 5.0f;
}

void AERNProjectileBase::Multicast_PlayImpactEffect_Implementation(FVector Location, FRotator Rotation)
{
	if (ImpactEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), ImpactEffect, Location, Rotation);
	}
}

void AERNProjectileBase::BeginPlay()
{
	Super::BeginPlay();

	// 가속 또는 속도 곡선 활성화 시 Tick 켬 (서버/클라 양쪽)
	if (bAccelerateOverTime || SpeedCurve)
	{
		SetActorTickEnabled(true);
	}

	// 곡선 사용 시 PMC MaxSpeed 제한 우회 (곡선이 속도 권한)
	if (SpeedCurve && ProjectileMovement)
	{
		ProjectileMovement->MaxSpeed = 99999.f;
	}

	if (!HasAuthority()) return;

	if (bHomingEnabled)
	{
		InitializeHoming();
	}

	// 서버에서만 랜덤 결정 - ChosenInitialDir 설정 시 클라로 리플리케이트되어 OnRep 발생
	if (bUseCustomInitialDirection)
	{
		const FVector RawDir(
			FMath::RandRange(MinInitialDirection.X, MaxInitialDirection.X),
			FMath::RandRange(MinInitialDirection.Y, MaxInitialDirection.Y),
			FMath::RandRange(MinInitialDirection.Z, MaxInitialDirection.Z)
		);
		ChosenInitialDir = RawDir.GetSafeNormal();
		ApplyInitialDirection(ChosenInitialDir);
	}
}

void AERNProjectileBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(AERNProjectileBase, ChosenInitialDir, COND_InitialOnly);
}

void AERNProjectileBase::OnRep_ChosenInitialDir()
{
	ApplyInitialDirection(ChosenInitialDir);
}

void AERNProjectileBase::ApplyInitialDirection(const FVector& LocalDir)
{
	if (!ProjectileMovement || LocalDir.IsNearlyZero()) return;

	// 변환 기준: Owner(슈터)의 Yaw만 사용 - SpawnRotation의 거리 의존 Pitch 누적 제거
	FRotator RefRot = GetActorRotation();
	if (AActor* Shooter = GetOwner())
	{
		const float ShooterYaw = Shooter->GetActorRotation().Yaw;
		RefRot = FRotator(0.f, ShooterYaw, 0.f);
	}

	const FVector WorldDir = RefRot.RotateVector(LocalDir);
	ProjectileMovement->Velocity = WorldDir * ProjectileMovement->InitialSpeed;

	if (ProjectileMovement->bRotationFollowsVelocity)
	{
		SetActorRotation(WorldDir.Rotation());
	}
}

void AERNProjectileBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (!ProjectileMovement) return;

	// 속도 곡선이 우선 - 매 Tick 절대값으로 덮어씀 (방향은 PMC/유도가 결정)
	if (SpeedCurve)
	{
		const float Elapsed = GetGameTimeSinceCreation();
		const float TargetSpeed = SpeedCurve->GetFloatValue(Elapsed);

		FVector Dir = ProjectileMovement->Velocity.GetSafeNormal();
		if (Dir.IsNearlyZero()) Dir = GetActorForwardVector();

		ProjectileMovement->Velocity = Dir * TargetSpeed;
		return;
	}

	// 단순 가속 (bAccelerateOverTime)
	FVector V = ProjectileMovement->Velocity;
	if (V.IsNearlyZero()) return;

	V += V.GetSafeNormal() * Acceleration * DeltaSeconds;

	// PMC MaxSpeed로 클램프 (BP에서 InitialSpeed보다 크게 설정해야 가속 효과 보임)
	if (ProjectileMovement->MaxSpeed > 0.f)
	{
		V = V.GetClampedToMaxSize(ProjectileMovement->MaxSpeed);
	}
	ProjectileMovement->Velocity = V;
}

void AERNProjectileBase::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (!HasAuthority()) return;

	// 폰은 Overlap으로 처리되므로 여기는 벽(WorldStatic/Dynamic) 충돌만
	Multicast_PlayImpactEffect(Hit.ImpactPoint, Hit.ImpactNormal.Rotation());
	Destroy();
}

void AERNProjectileBase::OnBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
	bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority()) return;
	if (!OtherActor || OtherActor == GetOwner()) return;

	AActor* OwnerActor = GetOwner();
	AProjectERNCharacter* PlayerShooter = Cast<AProjectERNCharacter>(OwnerActor);
	AERNEnemyCharacter* EnemyShooter = Cast<AERNEnemyCharacter>(OwnerActor);

	AProjectERNCharacter* OtherPlayer = Cast<AProjectERNCharacter>(OtherActor);
	AERNEnemyCharacter* OtherEnemy = Cast<AERNEnemyCharacter>(OtherActor);

	// 플레이어가 쏜 투사체 - 같은 플레이어는 통과, 적에게만 데미지
	if (PlayerShooter)
	{
		if (OtherPlayer) return;
		if (!OtherEnemy) return;

		OtherEnemy->TakeDamage(Damage, FDamageEvent(), GetInstigatorController(), OwnerActor);
		OtherEnemy->TryApplyStagger(StaggerPower);
	}
	// 적이 쏜 투사체 - 같은 적은 통과, 플레이어에게만 데미지
	else if (EnemyShooter)
	{
		if (OtherEnemy) return;
		if (!OtherPlayer) return;

		OtherPlayer->TakeDamage(Damage, FDamageEvent(), GetInstigatorController(), OwnerActor);
		OtherPlayer->TryApplyStagger(StaggerPower);
	}
	else
	{
		return;
	}

	const FVector ImpactPoint = bFromSweep ? FVector(SweepResult.ImpactPoint) : GetActorLocation();
	const FRotator ImpactRot = bFromSweep ? SweepResult.ImpactNormal.Rotation() : GetActorRotation();
	Multicast_PlayImpactEffect(ImpactPoint, ImpactRot);
	Destroy();
}

// 유도 투사체일 경우
void AERNProjectileBase::InitializeHoming()
{
	if (!ProjectileMovement) return;

	AERNEnemyCharacter* EnemyShooter = Cast<AERNEnemyCharacter>(GetOwner());

	AActor* Target = nullptr;
	if (EnemyShooter)
	{
		Target = GetEnemyBlackboardTarget();
	}
	else
	{
		// 플레이어 투사체: 락온 미구현 임시로 자동 검색
		Target = FindHomingTargetForPlayer();
	}

	if (Target && Target->GetRootComponent())
	{
		ProjectileMovement->bIsHomingProjectile = true;
		ProjectileMovement->HomingTargetComponent = Target->GetRootComponent();
		ProjectileMovement->HomingAccelerationMagnitude = HomingAcceleration;
	}
	// 타겟 못 찾으면 직선 비행
}

// 적 탐색 (임시)
AActor* AERNProjectileBase::FindHomingTargetForPlayer() const
{
	TArray<AActor*> Enemies;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AERNEnemyCharacter::StaticClass(), Enemies);

	AActor* Best = nullptr;
	float BestDistSq = HomingSearchRadius * HomingSearchRadius;

	const FVector Origin = GetActorLocation();
	const FVector Forward = GetActorForwardVector();
	const float CosAngle = FMath::Cos(FMath::DegreesToRadians(HomingSearchHalfAngle));

	for (AActor* E : Enemies)
	{
		if (!IsValid(E)) continue;

		const FVector ToTarget = E->GetActorLocation() - Origin;
		const float DistSq = ToTarget.SizeSquared();
		if (DistSq > BestDistSq) continue;

		if (FVector::DotProduct(Forward, ToTarget.GetSafeNormal()) < CosAngle) continue;

		BestDistSq = DistSq;
		Best = E;
	}
	return Best;
}

// 적의 AIC에서 타겟 액터 가져오기
AActor* AERNProjectileBase::GetEnemyBlackboardTarget() const
{
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn) return nullptr;

	AAIController* AIC = Cast<AAIController>(OwnerPawn->GetController());
	if (!AIC) return nullptr;

	UBlackboardComponent* BB = AIC->GetBlackboardComponent();
	if (!BB) return nullptr;

	return Cast<AActor>(BB->GetValueAsObject(TEXT("TargetActor")));
}
