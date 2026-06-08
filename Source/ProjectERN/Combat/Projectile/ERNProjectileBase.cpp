// Copyright Epic Games, Inc. All Rights Reserved.

#include "Combat/Projectile/ERNProjectileBase.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/KismetSystemLibrary.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Engine/DamageEvents.h"
#include "Kismet/GameplayStatics.h"
#include "Components/AudioComponent.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "Curves/CurveFloat.h"
#include "Character/Enemy/ERNEnemyCharacter.h"
#include "Character/Player/ProjectERNCharacter.h"
#include "Camera/CameraShakeBase.h"
#include "Camera/PlayerCameraManager.h"
#include "Inventory/Components/ERNEquipmentComponent.h"
#include "Combat/Weapons/ERNWeaponBase.h"
#include "GAS/ERNAttributeSet.h"
#include "AbilitySystemComponent.h"

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
	// 착탄 스케일 조정 적용을 위해 추가 변수 적용
	if (ImpactEffect)
	{
		UNiagaraComponent* ImpactComp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(),
			ImpactEffect,
			Location,
			Rotation,
			ImpactEffectScale,
			true,
			true,
			ENCPoolMethod::None,
			true);

		// 루핑/무한 Lifetime 이펙트가 자동 소멸 안 돼서 누적되는 것 방지 — 5초 후 강제 정리
		if (ImpactComp)
		{
			TWeakObjectPtr<UNiagaraComponent> WeakComp(ImpactComp);
			FTimerHandle CleanupHandle;
			GetWorld()->GetTimerManager().SetTimer(
				CleanupHandle,
				FTimerDelegate::CreateLambda([WeakComp]()
				{
					if (WeakComp.IsValid())
					{
						WeakComp->DestroyComponent();
					}
				}),
				5.f,
				false);
		}
	}
	
	if (ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this, ImpactSound, Location, Rotation,
			1.f, 1.f, 0.f, ImpactSoundAttenuation);
	}
}

void AERNProjectileBase::ClearHomingTargetingActor(UWorld* World, AActor* TargetActor)
{
	if (!World || !TargetActor)
	{
		return;
	}

	TArray<AActor*> Projectiles;
	UGameplayStatics::GetAllActorsOfClass(World, AERNProjectileBase::StaticClass(), Projectiles);

	for (AActor* Actor : Projectiles)
	{
		AERNProjectileBase* Projectile = Cast<AERNProjectileBase>(Actor);
		if (!Projectile || Projectile->HomingTarget.Get() != TargetActor)
		{
			continue;
		}

		// 유도 중단 → 직선으로 빠져나감
		if (Projectile->ProjectileMovement)
		{
			Projectile->ProjectileMovement->bIsHomingProjectile = false;
		}
		Projectile->HomingTarget.Reset();
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

	// 비행 사운드 재생 (서버/클라 모두)
	if (FlightSound)
	{
		FlightAudioComponent = UGameplayStatics::SpawnSoundAttached(
			FlightSound, RootComponent, NAME_None,
			FVector::ZeroVector, EAttachLocation::KeepRelativeOffset,
			true, 1.f, 1.f, 0.f, FlightSoundAttenuation);
	}

	if (!HasAuthority()) return;

	// 플레이어 소유 투사체면 owner의 공격력/무기 공격력으로 데미지 덮어쓰기
	RecalculateDamageFromOwner();

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
	DOREPLIFETIME(AERNProjectileBase, HomingTarget);
}

void AERNProjectileBase::OnRep_HomingTarget()
{
	if (HomingTarget.IsValid())
	{
		ApplyHomingSettings(HomingTarget.Get());
	}
}

void AERNProjectileBase::ApplyHomingSettings(AActor* Target)
{
	if (ProjectileMovement && Target && Target->GetRootComponent())
	{
		ProjectileMovement->bIsHomingProjectile = true;
		ProjectileMovement->HomingTargetComponent = Target->GetRootComponent();
		ProjectileMovement->HomingAccelerationMagnitude = HomingAcceleration;
	}
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

	// 폭발 투사체면 벽에 부딪혀도 범위 데미지 적용
	if (bExplode)
	{
		ApplyExplosionDamage(Hit.ImpactPoint);
	}

	Multicast_PlayImpactEffect(Hit.ImpactPoint, Hit.ImpactNormal.Rotation());
	SetLifeSpan(0.2f);	// 클라이언트에서도 보이도록 약간 지연
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

	// 같은 팀이면 통과
	// 부활 공격은 적용으로 수정
	if (PlayerShooter && OtherPlayer)
	{
		if (AProjectERNCharacter::TryApplyReviveHit(OtherPlayer, PlayerShooter->GetController()))
		{
			const FVector ImpactPoint = bFromSweep ? FVector(SweepResult.ImpactPoint) : GetActorLocation();

			const FRotator ImpactRot = bFromSweep ? SweepResult.ImpactNormal.Rotation() : GetActorRotation();

			Multicast_PlayImpactEffect(ImpactPoint, ImpactRot);
			SetLifeSpan(0.2f);
		}

		return;
	}
	
	if (EnemyShooter && OtherEnemy) return;

	// 유효한 타겟인지 확인
	if (PlayerShooter && !OtherEnemy) return;
	if (EnemyShooter && !OtherPlayer) return;
	if (!PlayerShooter && !EnemyShooter) return;

	const FVector ImpactPoint = bFromSweep ? FVector(SweepResult.ImpactPoint) : GetActorLocation();
	const FRotator ImpactRot = bFromSweep ? SweepResult.ImpactNormal.Rotation() : GetActorRotation();

	// 직격 데미지 적용
	if (OtherEnemy)
	{
		OtherEnemy->TakeDamage(Damage, FDamageEvent(), GetInstigatorController(), OwnerActor);
		// 투사체 충돌 지점을 HitOrigin으로 전달 → 적이 투사체 방향 기준 4방향 경직
		OtherEnemy->TryApplyStagger(StaggerPower, ImpactPoint);
	}
	else if (OtherPlayer)
	{
		OtherPlayer->TakeDamage(Damage, FDamageEvent(), GetInstigatorController(), OwnerActor);
		// 투사체 충돌 지점을 HitOrigin으로 전달 → 플레이어가 투사체 방향 기준 4방향 경직
		OtherPlayer->TryApplyStagger(StaggerPower, ImpactPoint);
	}

	// 직격 넉백 - 맞은 대상을 투사체 진행 방향 그대로 밀어냄
	if (bKnockbackEnabled)
	{
		const FVector KnockbackDir = (ProjectileMovement && !ProjectileMovement->Velocity.IsNearlyZero())
			? ProjectileMovement->Velocity
			: GetActorForwardVector();
		ApplyKnockback(Cast<ACharacter>(OtherActor), KnockbackDir, KnockbackForce);
	}

	// 폭발 투사체면 범위 데미지 추가 적용
	if (bExplode)
	{
		ApplyExplosionDamage(ImpactPoint);
	}

	Multicast_PlayImpactEffect(ImpactPoint, ImpactRot);
	SetLifeSpan(0.2f);	// 클라이언트에서도 보이도록 약간 지연
}

// 유도 투사체일 경우
void AERNProjectileBase::InitializeHoming()
{
	if (!ProjectileMovement) return;

	// 외부(AnimNotify 등)에서 이미 주입된 타겟이 있으면 우선 사용
	AActor* Target = HomingTarget.Get();

	if (!Target)
	{
		AERNEnemyCharacter* EnemyShooter = Cast<AERNEnemyCharacter>(GetOwner());
		if (EnemyShooter)
		{
			Target = GetEnemyBlackboardTarget();
		}
		else
		{
			// 플레이어 투사체: 락온 미구현 임시로 자동 검색
			Target = FindHomingTargetForPlayer();
		}
	}

	if (Target)
	{
		HomingTarget = Target;  // 리플리케이트 -> 클라에서 OnRep 호출
		ApplyHomingSettings(Target);
	}
	// 타겟 못 찾으면 직선 비행
}

// 적 탐색 (임시)
AActor* AERNProjectileBase::FindHomingTargetForPlayer() const
{
	/*
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
	*/

	AActor* Best = nullptr;
	float BestDistSq = HomingSearchRadius * HomingSearchRadius;

	const FVector Origin = GetActorLocation();
	const FVector Forward = GetActorForwardVector();
	const float CosAngle = FMath::Cos(FMath::DegreesToRadians(HomingSearchHalfAngle));

	auto ConsiderTarget = [&](AActor* Candidate)
	{
		if (!IsValid(Candidate) || Candidate == GetOwner())
		{
			return;
		}

		const FVector ToTarget = Candidate->GetActorLocation() - Origin;
		const float DistSq = ToTarget.SizeSquared();

		if (DistSq > BestDistSq)
		{
			return;
		}

		if (FVector::DotProduct(Forward, ToTarget.GetSafeNormal()) < CosAngle)
		{
			return;
		}

		BestDistSq = DistSq;
		Best = Candidate;
	};

	TArray<AActor*> Enemies;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AERNEnemyCharacter::StaticClass(), Enemies);

	for (AActor* Actor : Enemies)
	{
		AERNEnemyCharacter* Enemy = Cast<AERNEnemyCharacter>(Actor);
		if (!Enemy || !Enemy->IsTargetable())
		{
			continue;
		}

		ConsiderTarget(Enemy);
	}

	// 부활용 아군 호밍 기능 추가
	TArray<AActor*> Players;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AProjectERNCharacter::StaticClass(),Players);

	for (AActor* Actor : Players)
	{
		AProjectERNCharacter* Player = Cast<AProjectERNCharacter>(Actor);
		if (!Player || !Player->IsDowned())
		{
			continue;
		}

		ConsiderTarget(Player);
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

// 플레이어 owner 기준 직격/폭발 데미지 재계산
void AERNProjectileBase::RecalculateDamageFromOwner()
{
	AProjectERNCharacter* PlayerOwner = Cast<AProjectERNCharacter>(GetOwner());
	if (!PlayerOwner)
	{
		// 적/논플레이어 투사체는 BP에 설정된 Damage 그대로 사용
		return;
	}

	float CharacterAP = 0.f;
	if (UAbilitySystemComponent* ASC = PlayerOwner->GetAbilitySystemComponent())
	{
		CharacterAP = ASC->GetNumericAttribute(UERNAttributeSet::GetAttackPowerAttribute());
	}

	float WeaponDmg = 0.f;
	if (UERNEquipmentComponent* Equip = PlayerOwner->GetEquipmentComponent())
	{
		if (AERNWeaponBase* Weapon = Equip->CurrentWeapon)
		{
			WeaponDmg = Weapon->LightAttackDamage;
		}
	}

	Damage = CharacterAP * AttackPowerScale + WeaponDmg * WeaponDamageScale;

	if (bExplode)
	{
		ExplosionDamage = CharacterAP * ExplosionAttackPowerScale + WeaponDmg * ExplosionWeaponDamageScale;
	}
}

// 폭발 범위 데미지 적용
void AERNProjectileBase::ApplyExplosionDamage(const FVector& ExplosionCenter)
{
	AActor* OwnerActor = GetOwner();
	AProjectERNCharacter* PlayerShooter = Cast<AProjectERNCharacter>(OwnerActor);
	AERNEnemyCharacter* EnemyShooter = Cast<AERNEnemyCharacter>(OwnerActor);

	TArray<AActor*> IgnoreActors;
	IgnoreActors.Add(this);
	if (OwnerActor) IgnoreActors.Add(OwnerActor);

	TArray<AActor*> OverlappedActors;
	UKismetSystemLibrary::SphereOverlapActors(
		this, ExplosionCenter, ExplosionRadius,
		TArray<TEnumAsByte<EObjectTypeQuery>>(), APawn::StaticClass(),
		IgnoreActors, OverlappedActors);

	for (AActor* HitActor : OverlappedActors)
	{
		if (!HitActor) continue;

		// 플레이어가 쏜 폭발 - 적에게만 데미지
		if (PlayerShooter)
		{
			// 기절한 아군에게도 적용
			if (AProjectERNCharacter* Player = Cast<AProjectERNCharacter>(HitActor))
			{
				if (AProjectERNCharacter::TryApplyReviveHit(Player, PlayerShooter->GetController()))
				{
					continue;
				}
			}
			
			if (AERNEnemyCharacter* Enemy = Cast<AERNEnemyCharacter>(HitActor))
			{
				Enemy->TakeDamage(ExplosionDamage, FDamageEvent(), GetInstigatorController(), OwnerActor);
				// 폭발 중심을 HitOrigin으로 전달 → 적이 폭발 방향 기준 4방향 경직
				Enemy->TryApplyStagger(ExplosionStaggerPower, ExplosionCenter);

				// 폭발 넉백 - 폭발 중심에서 바깥(방사형)으로 밀어냄
				if (bKnockbackEnabled)
				{
					ApplyKnockback(Enemy, HitActor->GetActorLocation() - ExplosionCenter, ExplosionKnockbackForce);
				}
			}
		}
		// 적이 쏜 폭발 - 플레이어에게만 데미지
		else if (EnemyShooter)
		{
			if (AProjectERNCharacter* Player = Cast<AProjectERNCharacter>(HitActor))
			{
				Player->TakeDamage(ExplosionDamage, FDamageEvent(), GetInstigatorController(), OwnerActor);
				// 폭발 중심을 HitOrigin으로 전달 → 플레이어가 폭발 방향 기준 4방향 경직
				Player->TryApplyStagger(ExplosionStaggerPower, ExplosionCenter);

				// 폭발 넉백 - 폭발 중심에서 바깥(방사형)으로 밀어냄
				if (bKnockbackEnabled)
				{
					ApplyKnockback(Player, HitActor->GetActorLocation() - ExplosionCenter, ExplosionKnockbackForce);
				}
			}
		}
	}

	// 폭발 카메라 흔들림 (반경 내 모든 플레이어, 거리 감쇠)
	if (ExplosionShakeClass)
	{
		Multicast_PlayExplosionShake(ExplosionCenter);
	}
}

// 대상 캐릭터를 지정 방향으로 밀어냄 (서버 권위)
void AERNProjectileBase::ApplyKnockback(ACharacter* TargetCharacter, const FVector& Direction, float Force)
{
	if (!TargetCharacter || Force <= 0.f) return;

	const FVector LaunchDir = Direction.GetSafeNormal();
	if (LaunchDir.IsNearlyZero()) return;

	// 서버에서 호출 - CharacterMovementComponent가 PendingLaunchVelocity를 클라로 복제
	TargetCharacter->LaunchCharacter(LaunchDir * Force, true, true);
}

void AERNProjectileBase::Multicast_PlayExplosionShake_Implementation(FVector Origin)
{
	if (!ExplosionShakeClass) return;

	UWorld* World = GetWorld();
	if (!World) return;

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (!PC || !PC->PlayerCameraManager) continue;

		const FVector CamLoc = PC->PlayerCameraManager->GetCameraLocation();
		const float Dist = FVector::Dist(CamLoc, Origin);

		float Attenuation = 0.f;
		if (Dist <= ExplosionShakeInnerRadius)
		{
			Attenuation = 1.f;
		}
		else if (Dist < ExplosionShakeOuterRadius)
		{
			const float Alpha = (Dist - ExplosionShakeInnerRadius) / (ExplosionShakeOuterRadius - ExplosionShakeInnerRadius);
			Attenuation = FMath::Pow(1.f - Alpha, ExplosionShakeFalloff);
		}

		if (Attenuation <= 0.f) continue;

		PC->PlayerCameraManager->StartCameraShake(ExplosionShakeClass, ExplosionShakeScale * Attenuation);
	}
}
