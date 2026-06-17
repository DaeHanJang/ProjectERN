// Copyright Epic Games, Inc. All Rights Reserved.

#include "Combat/Projectile/ERNThunderCarrier.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Character/Enemy/ERNEnemyCharacter.h"
#include "Character/Player/ProjectERNCharacter.h"
#include "Character/ERNCharacterBase.h"
#include "Combat/ERNSkillDamageLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "NiagaraComponent.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"

namespace
{
	// 죽은 캐릭터인지 (적은 사망해도 잠시 액터가 남아 IsValid가 true이므로 별도 판별 필요)
	bool IsDeadCharacter(const AActor* Actor)
	{
		const AERNCharacterBase* Char = Cast<AERNCharacterBase>(Actor);
		return Char && Char->IsDead();
	}
}

AERNThunderCarrier::AERNThunderCarrier()
{
	// 캐리어는 직접 충돌하지 않는 순수 스포너 - Duration 타이머로만 소멸
	if (CollisionComponent)
	{
		CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	// 플레이어 위 유도용 좌표
	HomingProxy = CreateDefaultSubobject<USceneComponent>(TEXT("HomingProxy"));
	HomingProxy->SetupAttachment(RootComponent);
	HomingProxy->SetUsingAbsoluteLocation(true);

	// 천천히 따라가는 속도
	if (ProjectileMovement)
	{
		ProjectileMovement->InitialSpeed = 300.f;
		ProjectileMovement->MaxSpeed = 300.f;
	}

	// 유도 활성화 (프록시 추적)
	bHomingEnabled = true;
	HomingAcceleration = 1200.f;

	// HomingProxy 위치 갱신용 Tick
	PrimaryActorTick.bStartWithTickEnabled = true;

	// Duration은 자체 타이머로 관리
	InitialLifeSpan = 0.f;
}

void AERNThunderCarrier::BeginPlay()
{
	// 추적 안 하면 베이스 유도 초기화도 막음 (Super 전에 설정 → 직진은 베이스 PMC/초기방향이 처리)
	if (!bFollowTarget)
	{
		bHomingEnabled = false;
	}

	Super::BeginPlay();

	// 추적 모드일 때만 유도 셋업 (HomingProxy 추적)
	if (bFollowTarget && ProjectileMovement && HomingProxy)
	{
		ProjectileMovement->bIsHomingProjectile = true;
		ProjectileMovement->HomingTargetComponent = HomingProxy;
		ProjectileMovement->HomingAccelerationMagnitude = HomingAcceleration;
	}

	if (!HasAuthority()) return;

	// 추적 모드인데 노티파이가 타겟을 안 넣었으면 폴백
	if (bFollowTarget && !HomingTarget.IsValid())
	{
		EnsureValidTarget();
	}

	UWorld* World = GetWorld();
	if (!World) return;

	World->GetTimerManager().SetTimer(
		FireTimerHandle, this, &AERNThunderCarrier::OnFireTick,
		FireInterval, true, InitialFireDelay);

	World->GetTimerManager().SetTimer(
		DurationTimerHandle, this, &AERNThunderCarrier::OnDurationEnded,
		Duration, false);

	// 세대 캡 미만이면 분열 예약 (딱 한 번)
	if (bSplitEnabled && SplitGeneration < MaxSplitGenerations)
	{
		World->GetTimerManager().SetTimer(
			SplitTimerHandle, this, &AERNThunderCarrier::DoSplit,
			SplitDelay, false);
	}
}

void AERNThunderCarrier::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FireTimerHandle);
		World->GetTimerManager().ClearTimer(DurationTimerHandle);
		World->GetTimerManager().ClearTimer(SplitTimerHandle);
	}

	Super::EndPlay(EndPlayReason);
}

void AERNThunderCarrier::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// 추적 모드가 아니면 직진 (프록시 갱신 불필요)
	if (!bFollowTarget) return;

	// 타겟 플레이어 위로 위치 갱신 (서버/클라 양쪽)
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

void AERNThunderCarrier::OnFireTick()
{
	if (!HasAuthority()) return;

	// 추적 모드: 현재 타겟이 죽었거나 사라졌으면 가장 가까운 살아있는 적으로 재탐색
	if (bFollowTarget)
	{
		AActor* CurrentTarget = HomingTarget.Get();
		if (!IsValid(CurrentTarget) || IsDeadCharacter(CurrentTarget))
		{
			EnsureValidTarget();
		}

		// 재탐색 후에도 유효한(살아있는) 타겟이 없으면 이번 발사 스킵
		if (!HomingTarget.IsValid() || IsDeadCharacter(HomingTarget.Get()))
		{
			return;
		}
	}

	UWorld* World = GetWorld();
	if (!World) return;

	const FVector SpawnLoc = GetActorLocation();

	// StrikeProjectileClass가 할당돼 있으면 기존 액터 스폰 방식 (비교용)
	if (StrikeProjectileClass)
	{
		FActorSpawnParameters Params;
		Params.Owner = GetOwner();			// 보스 (팀/데미지 판정용)
		Params.Instigator = GetInstigator();
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		// 부모(캐리어) 위치에서 아래로 떨어뜨림 - 속도는 자식 BP가 결정 (전방 = 아래)
		const FRotator SpawnRot(-90.f, 0.f, 0.f);

		AERNProjectileBase* Strike = World->SpawnActor<AERNProjectileBase>(
			StrikeProjectileClass, SpawnLoc, SpawnRot, Params);

		if (Strike)
		{
			// 동적 난이도 출력 배율 적용 (보스 owner의 BP Damage에 곱함)
			if (AERNEnemyCharacter* BossOwner = Cast<AERNEnemyCharacter>(GetOwner()))
			{
				Strike->Damage *= BossOwner->OutgoingDamageMultiplier;
				Strike->ExplosionDamage *= BossOwner->OutgoingDamageMultiplier;
			}
		}

		Multicast_PlayFireEffect(SpawnLoc);
		return;
	}

	// StrikeProjectileClass가 비어 있으면 히트스캔 폭발 방식 (액터 스폰 없음)
	// 트레이스 방향: 기본은 정직하게 아래, 옵션 켜지면 월드 범위에서 랜덤
	FVector TraceDir(0.f, 0.f, -1.f);
	if (bRandomizeStrikeDirection)
	{
		const FVector RawDir(
			FMath::RandRange(MinStrikeDirection.X, MaxStrikeDirection.X),
			FMath::RandRange(MinStrikeDirection.Y, MaxStrikeDirection.Y),
			FMath::RandRange(MinStrikeDirection.Z, MaxStrikeDirection.Z));
		TraceDir = RawDir.GetSafeNormal();
		if (TraceDir.IsNearlyZero())
		{
			TraceDir = FVector(0.f, 0.f, -1.f);
		}
	}

	// 라인 트레이스 → 충돌 지점(없으면 끝점)을 폭발 중심으로
	const FVector TraceEnd = SpawnLoc + TraceDir * StrikeTraceDistance;
	FHitResult Hit;
	FCollisionQueryParams TraceParams(SCENE_QUERY_STAT(ThunderStrikeTrace), false, this);
	if (GetOwner()) TraceParams.AddIgnoredActor(GetOwner());
	const bool bHit = World->LineTraceSingleByChannel(Hit, SpawnLoc, TraceEnd, ECC_Visibility, TraceParams);
	const FVector ImpactPoint = bHit ? Hit.ImpactPoint : TraceEnd;

	// 데미지 계산: 스킬 스케일 모드(플레이어)면 공격력/무기 데미지 반영, 아니면 기존 보스 방식(플랫 × 난이도 배율)
	float FinalDamage;
	if (bUseSkillDamageForStrike)
	{
		FinalDamage = UERNSkillDamageLibrary::CalculateSkillDamage(GetOwner(), StrikeSkillDamage);
	}
	else
	{
		FinalDamage = StrikeExplosionDamage;
		if (AERNEnemyCharacter* BossOwner = Cast<AERNEnemyCharacter>(GetOwner()))
		{
			FinalDamage *= BossOwner->OutgoingDamageMultiplier;
		}
	}

	// 기존 projectilebase와 동일한 폭발 공식 (공유 정적 헬퍼)
	ApplyRadialExplosion(
		this, GetOwner(), GetInstigatorController(), ImpactPoint,
		StrikeExplosionRadius, FinalDamage, StrikeStaggerPower,
		bStrikeAddMaxHealthPercentDamage, StrikeMaxHealthPercentDamage,
		bStrikeKnockback, StrikeKnockbackForce);

	// 비주얼: 발사(머즐, 캐리어 위치) + 임팩트(번개, 충돌 지점)
	Multicast_PlayFireEffect(SpawnLoc);
	Multicast_PlayStrikeImpact(ImpactPoint);
}

void AERNThunderCarrier::Multicast_PlayFireEffect_Implementation(FVector Location)
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

void AERNThunderCarrier::Multicast_PlayStrikeImpact_Implementation(FVector Location)
{
	if (StrikeImpactEffect)
	{
		UNiagaraComponent* ImpactComp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(),
			StrikeImpactEffect,
			Location,
			FRotator::ZeroRotator,
			FVector(1.f),
			true,
			true,
			ENCPoolMethod::None,
			true);

		// 루핑/무한 Lifetime 이펙트가 자동 소멸 안 돼서 누적되는 것 방지 — 일정 시간 후 강제 정리
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
				StrikeImpactLifetime,
				false);
		}
	}
	if (StrikeImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this, StrikeImpactSound, Location, FRotator::ZeroRotator,
			1.f, 1.f, 0.f, FireSoundAttenuation);
	}
}

void AERNThunderCarrier::OnDurationEnded()
{
	if (!HasAuthority()) return;

	SetLifeSpan(0.1f);	// 클라이언트 정리 시간 확보
}

void AERNThunderCarrier::DoSplit()
{
	if (!HasAuthority() || !ProjectileMovement) return;

	UWorld* World = GetWorld();
	if (!World) return;

	// 현재 진행 방향/속도 기준 (정지 상태면 forward 폴백)
	float Speed = ProjectileMovement->Velocity.Size();
	FVector BaseDir = ProjectileMovement->Velocity.GetSafeNormal();
	if (BaseDir.IsNearlyZero())
	{
		BaseDir = GetActorForwardVector();
	}
	if (Speed <= 1.f)
	{
		Speed = ProjectileMovement->InitialSpeed;
	}

	const float Angles[] = { SplitAngle, -SplitAngle };
	for (float Angle : Angles)
	{
		const FVector CloneDir = BaseDir.RotateAngleAxis(Angle, FVector::UpVector);
		const FTransform SpawnTM(CloneDir.Rotation(), GetActorLocation());

		// 같은 클래스(BP)로 디퍼드 스폰 → 세대 주입 후 FinishSpawning
		AERNThunderCarrier* Clone = World->SpawnActorDeferred<AERNThunderCarrier>(
			GetClass(), SpawnTM, GetOwner(), GetInstigator());
		if (!Clone) continue;

		Clone->SplitGeneration = SplitGeneration + 1;	// 세대 +1 → 캡 도달 시 더 안 쪼개짐
		Clone->bFollowTarget = false;					// 분열 클론은 직진
		Clone->FinishSpawning(SpawnTM);

		// 진행 방향 직접 세팅 (베이스 초기방향 로직에 안 휘둘림)
		if (Clone->ProjectileMovement)
		{
			Clone->ProjectileMovement->Velocity = CloneDir * Speed;
		}
	}
}

void AERNThunderCarrier::EnsureValidTarget()
{
	AActor* NewTarget = FindNearestTarget();
	if (NewTarget)
	{
		HomingTarget = NewTarget;	// 리플리케이트
	}
}

AActor* AERNThunderCarrier::FindNearestTarget() const
{
	// 소유자가 적(보스/몹)인지 판별 → 노릴 대상 팀 결정
	const bool bOwnerIsEnemy = (Cast<AERNEnemyCharacter>(GetOwner()) != nullptr);

	// 적이 소유한 경우 블랙보드 주 타겟 우선 (보스 AI가 지정한 타겟)
	if (bOwnerIsEnemy)
	{
		if (AActor* BBTarget = GetEnemyBlackboardTarget())
		{
			return BBTarget;
		}
	}

	// 적이 소유 → 플레이어를, 플레이어가 소유 → 적을 검색
	UClass* HostileClass = bOwnerIsEnemy
		? static_cast<UClass*>(AProjectERNCharacter::StaticClass())
		: static_cast<UClass*>(AERNEnemyCharacter::StaticClass());

	TArray<AActor*> Candidates;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), HostileClass, Candidates);

	AActor* Best = nullptr;
	float BestDistSq = TargetSearchRadius * TargetSearchRadius;
	const FVector Origin = GetActorLocation();

	for (AActor* C : Candidates)
	{
		// 살아있는 대상만 (IsDead는 공용 베이스 AERNCharacterBase에 정의)
		AERNCharacterBase* Char = Cast<AERNCharacterBase>(C);
		if (!Char || Char->IsDead()) continue;

		const float DistSq = FVector::DistSquared(Origin, Char->GetActorLocation());
		if (DistSq > BestDistSq) continue;

		BestDistSq = DistSq;
		Best = C;
	}
	return Best;
}
