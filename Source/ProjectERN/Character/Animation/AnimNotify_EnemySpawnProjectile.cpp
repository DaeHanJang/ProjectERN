// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/Animation/AnimNotify_EnemySpawnProjectile.h"
#include "Character/Enemy/ERNEnemyCharacter.h"
#include "Character/Enemy/AI/ERNMobAIController.h"
#include "Character/Enemy/AI/ERNBossAIController.h"
#include "Combat/Projectile/ERNProjectileBase.h"
#include "AIController.h"

void UAnimNotify_EnemySpawnProjectile::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	AERNEnemyCharacter* Enemy = MeshComp ? Cast<AERNEnemyCharacter>(MeshComp->GetOwner()) : nullptr;
	if (!Enemy || !Enemy->HasAuthority()) return;

	// 태그 일치하는 투사체 설정 검색
	const FEnemyProjectileConfig* Config = Enemy->ProjectileConfigs.FindByPredicate(
		[this](const FEnemyProjectileConfig& C) { return C.Tag == ProjectileTag; }
	);

	if (!Config || !Config->ProjectileClass) return;

	// 소켓 위치
	FVector SpawnLocation = Enemy->GetMesh()->GetSocketLocation(Config->SpawnSocket);

	AAIController* AIC = Cast<AAIController>(Enemy->GetController());
	AActor* TargetActor = nullptr;

	// bRandomTarget이면 감지중인 플레이어 중 랜덤 선택 (Mob/Boss 양쪽 지원)
	if (bRandomTarget)
	{
		TArray<AActor*> PerceivedPlayers;

		AERNMobAIController* MobAIC = Cast<AERNMobAIController>(AIC);
		if (MobAIC)
		{
			PerceivedPlayers = MobAIC->GetPerceivedPlayers();
		}
		else
		{
			AERNBossAIController* BossAIC = Cast<AERNBossAIController>(AIC);
			if (BossAIC)
			{
				PerceivedPlayers = BossAIC->GetPerceivedPlayers();
			}
		}

		if (PerceivedPlayers.Num() > 0)
		{
			const int32 Index = FMath::RandRange(0, PerceivedPlayers.Num() - 1);
			TargetActor = PerceivedPlayers[Index];
		}
	}

	// 폴백: 기존 FocusActor (랜덤 선택 실패 또는 bRandomTarget=false)
	if (!TargetActor && AIC)
	{
		TargetActor = AIC->GetFocusActor();
	}

	FRotator SpawnRotation = TargetActor ?
		(TargetActor->GetActorLocation() - SpawnLocation).GetSafeNormal().Rotation()
		: Enemy->GetActorRotation();

	// Deferred 스폰 → 호밍 타겟 사전 주입 → FinishSpawning
	const FTransform SpawnTransform(SpawnRotation, SpawnLocation);
	AERNProjectileBase* Projectile = Enemy->GetWorld()->SpawnActorDeferred<AERNProjectileBase>(
		Config->ProjectileClass, SpawnTransform, Enemy, Enemy);

	if (Projectile)
	{
		// bRandomTarget으로 골라진 타겟이 있으면 호밍 타겟으로도 미리 주입
		// (InitializeHoming이 BeginPlay에서 블랙보드 타겟으로 덮어쓰는 것 방지)
		if (bRandomTarget && TargetActor)
		{
			Projectile->HomingTarget = TargetActor;
		}

		// 동적 난이도 출력 배율 적용 (적 투사체는 BP 설정 Damage를 그대로 쓰므로 여기서 곱함. 잡몹은 1.0)
		Projectile->Damage *= Enemy->OutgoingDamageMultiplier;
		Projectile->ExplosionDamage *= Enemy->OutgoingDamageMultiplier;

		Projectile->FinishSpawning(SpawnTransform);
	}
}
