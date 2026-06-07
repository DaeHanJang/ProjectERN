// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/Animation/AnimNotify_EnemySpawnProjectilePerPlayer.h"
#include "Character/Enemy/ERNEnemyCharacter.h"
#include "Character/Enemy/AI/ERNMobAIController.h"
#include "Character/Enemy/AI/ERNBossAIController.h"
#include "Character/Player/ProjectERNCharacter.h"
#include "Combat/Projectile/ERNProjectileBase.h"
#include "AIController.h"

void UAnimNotify_EnemySpawnProjectilePerPlayer::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation,
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
