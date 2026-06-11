// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/Animation/AnimNotifyState_EnemyDamageCounterProjectile.h"
#include "Character/Enemy/ERNEnemyCharacter.h"
#include "Character/Enemy/AI/ERNMobAIController.h"
#include "Character/Enemy/AI/ERNBossAIController.h"
#include "Character/Player/ProjectERNCharacter.h"
#include "Combat/Projectile/ERNProjectileBase.h"
#include "AIController.h"
#include "TimerManager.h"

void UAnimNotifyState_EnemyDamageCounterProjectile::NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	float TotalDuration, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	AERNEnemyCharacter* Enemy = MeshComp ? Cast<AERNEnemyCharacter>(MeshComp->GetOwner()) : nullptr;
	if (!Enemy || !Enemy->HasAuthority()) return;

	Enemy->StartDamageAccumulation();
}

void UAnimNotifyState_EnemyDamageCounterProjectile::NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	float FrameDeltaTime, const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyTick(MeshComp, Animation, FrameDeltaTime, EventReference);

	AERNEnemyCharacter* Enemy = MeshComp ? Cast<AERNEnemyCharacter>(MeshComp->GetOwner()) : nullptr;
	if (!Enemy || !Enemy->HasAuthority()) return;

	// 임계치 도달 시 차지 연출 → FireDelay 후 1회 발사. 추적은 즉시 종료 (누적 0 리셋 → 구간 내 재발사 없음)
	if (DamageThreshold > 0.f && Enemy->GetAccumulatedDamage() >= DamageThreshold)
	{
		Enemy->StopDamageAccumulation();

		// 차지 연출 - 임계치 도달 순간의 스폰 소켓 위치에서 모든 클라이언트에 재생
		const FEnemyProjectileConfig* Config = Enemy->ProjectileConfigs.FindByPredicate(
			[this](const FEnemyProjectileConfig& C) { return C.Tag == ProjectileTag; }
		);
		const FVector FXLocation = Config
			? Enemy->GetMesh()->GetSocketLocation(Config->SpawnSocket)
			: Enemy->GetActorLocation();
		Enemy->Multicast_PlayCounterChargeFX(ChargeNiagara, ChargeSound, FXLocation);

		if (FireDelay > 0.f)
		{
			// 구간 종료와 무관하게 FireDelay 후 발사. 적이 소멸하면 WeakLambda가 자동 무효화
			FTimerHandle TimerHandle;
			Enemy->GetWorld()->GetTimerManager().SetTimer(TimerHandle,
				FTimerDelegate::CreateWeakLambda(Enemy, [this, Enemy]()
				{
					if (!Enemy->IsDead())
					{
						SpawnProjectilePerPlayer(Enemy);
					}
				}),
				FireDelay, false);
		}
		else
		{
			SpawnProjectilePerPlayer(Enemy);
		}
	}
}

void UAnimNotifyState_EnemyDamageCounterProjectile::NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	AERNEnemyCharacter* Enemy = MeshComp ? Cast<AERNEnemyCharacter>(MeshComp->GetOwner()) : nullptr;
	if (!Enemy || !Enemy->HasAuthority()) return;

	Enemy->StopDamageAccumulation();
}

void UAnimNotifyState_EnemyDamageCounterProjectile::SpawnProjectilePerPlayer(AERNEnemyCharacter* Enemy) const
{
	// 태그 일치하는 투사체 설정 검색
	const FEnemyProjectileConfig* Config = Enemy->ProjectileConfigs.FindByPredicate(
		[this](const FEnemyProjectileConfig& C) { return C.Tag == ProjectileTag; }
	);

	if (!Config || !Config->ProjectileClass) return;

	// 소켓 위치 (모든 투사체 동일 위치에서 스폰 후 각자 타겟으로 이동)
	const FVector SpawnLocation = Enemy->GetMesh()->GetSocketLocation(Config->SpawnSocket);

	AAIController* AIC = Cast<AAIController>(Enemy->GetController());

	// 감지중인 플레이어 수집 (Mob/Boss 양쪽 지원)
	TArray<AActor*> Players;

	AERNMobAIController* MobAIC = Cast<AERNMobAIController>(AIC);
	if (MobAIC)
	{
		Players = MobAIC->GetPerceivedPlayers();
	}
	else
	{
		AERNBossAIController* BossAIC = Cast<AERNBossAIController>(AIC);
		if (BossAIC)
		{
			Players = BossAIC->GetPerceivedPlayers();
		}
	}

	// 폴백: 감지 목록이 비면 주 어그로(FocusActor) 한 명
	if (Players.Num() == 0 && AIC)
	{
		if (AActor* Focus = AIC->GetFocusActor())
		{
			Players.Add(Focus);
		}
	}

	// 플레이어 한 명당 투사체 하나 스폰
	for (AActor* PlayerActor : Players)
	{
		AProjectERNCharacter* Player = Cast<AProjectERNCharacter>(PlayerActor);
		if (!Player || Player->IsDead()) continue;

		const FRotator SpawnRotation = (Player->GetActorLocation() - SpawnLocation).GetSafeNormal().Rotation();
		const FTransform SpawnTransform(SpawnRotation, SpawnLocation);

		// Deferred 스폰 → 추적 타겟 사전 주입 → FinishSpawning
		AERNProjectileBase* Projectile = Enemy->GetWorld()->SpawnActorDeferred<AERNProjectileBase>(
			Config->ProjectileClass, SpawnTransform, Enemy, Enemy);

		if (Projectile)
		{
			// 이 투사체가 추적할 플레이어 주입 (BeginPlay 전)
			Projectile->HomingTarget = Player;
			Projectile->FinishSpawning(SpawnTransform);
		}
	}
}
