// Copyright Epic Games, Inc. All Rights Reserved.

#include "Combat/Projectile/ERNSlashFreezeProjectile.h"
#include "Character/Player/ProjectERNCharacter.h"

bool AERNSlashFreezeProjectile::HandlePlayerHit(AProjectERNCharacter* HitPlayer, const FVector& ImpactPoint)
{
	// 서버 권위에서만 동작 (OnBeginOverlap이 서버에서만 호출되지만 방어적으로 한 번 더 확인)
	if (!HasAuthority() || !HitPlayer)
	{
		return false;
	}

	// 즉시 데미지/경직 대신 프리즈 → 프리즈 종료 시 데미지 + 경직(히트리액트) + 사운드 적용
	const float DelayedDamage = Damage + GetMaxHealthBonusDamage(HitPlayer);
	HitPlayer->ApplySliceFreeze(DelayedDamage, StaggerPower, ImpactPoint, FreezeDuration,
		GetInstigatorController(), GetOwner(), DelayedDamageSound, DelayedDamageSoundAttenuation);

	// true 반환 → 베이스 OnBeginOverlap의 기본 직격 데미지/경직 처리 스킵
	return true;
}
