// Copyright Epic Games, Inc. All Rights Reserved.

#include "Combat/Projectile/ERNSlashFreezeProjectile.h"
#include "Character/Player/ProjectERNCharacter.h"
#include "Components/SkeletalMeshComponent.h"

AERNSlashFreezeProjectile::AERNSlashFreezeProjectile()
{
	SkeletalMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SkeletalMesh"));
	SkeletalMesh->SetupAttachment(RootComponent);

	// 비주얼 전용 - 충돌/오버랩은 베이스의 CollisionComponent가 담당하므로 끈다.
	SkeletalMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SkeletalMesh->SetGenerateOverlapEvents(false);
}

bool AERNSlashFreezeProjectile::HandlePlayerHit(AProjectERNCharacter* HitPlayer, const FVector& ImpactPoint)
{
	// 서버 권위에서만 동작 (OnBeginOverlap이 서버에서만 호출되지만 방어적으로 한 번 더 확인)
	if (!HasAuthority() || !HitPlayer)
	{
		return false;
	}

	// 즉시 데미지/경직 대신 프리즈 → 프리즈 종료 시 데미지 + 경직(히트리액트) + 사운드/이펙트 적용
	const float DelayedDamage = Damage + GetMaxHealthBonusDamage(HitPlayer);
	HitPlayer->ApplySliceFreeze(DelayedDamage, StaggerPower, ImpactPoint, FreezeDuration,
		GetInstigatorController(), GetOwner(), DelayedDamageSound, DelayedDamageSoundAttenuation,
		DelayedDamageEffect);

	// true 반환 → 베이스 OnBeginOverlap의 기본 직격 데미지/경직 처리 스킵
	return true;
}
