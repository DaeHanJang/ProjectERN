// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/Animation/AnimNotify_EnemySpawnProjectile.h"
#include "Character/Enemy/ERNEnemyCharacter.h"
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
	
	// AIC에서 Focus중인 액터 방향
	AAIController* AIC = Cast<AAIController>(Enemy->GetController());
	AActor* FocusActor = AIC ? AIC->GetFocusActor() : nullptr;
	
	FRotator SpawnRotation = FocusActor ?
		(FocusActor->GetActorLocation() - SpawnLocation).GetSafeNormal().Rotation()
		: Enemy->GetActorRotation();

	FActorSpawnParameters Params;
	Params.Instigator = Enemy;
	Params.Owner = Enemy;

	Enemy->GetWorld()->SpawnActor<AERNProjectileBase>(Config->ProjectileClass, SpawnLocation, SpawnRotation, Params);
}
