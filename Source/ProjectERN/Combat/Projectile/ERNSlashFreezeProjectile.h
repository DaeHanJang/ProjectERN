// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Combat/Projectile/ERNProjectileBase.h"
#include "ERNSlashFreezeProjectile.generated.h"

/**
 * AERNSlashFreezeProjectile - 보스가 플레이어에게 사용하는 "쾌속 베기" 투사체.
 * 직격 시 즉시 데미지 대신, 맞은 플레이어를 일정 시간 이동 불가(프리즈) 상태로 만들고
 * 화면 사선 슬라이스 연출을 띄운다. 프리즈가 끝나는 순간 지연 데미지가 들어간다.
 */
UCLASS()
class PROJECTERN_API AERNSlashFreezeProjectile : public AERNProjectileBase
{
	GENERATED_BODY()

protected:
	// 직격 가로채기 - 즉시 데미지 대신 프리즈 + 지연 데미지를 적용하고 true 반환
	virtual bool HandlePlayerHit(AProjectERNCharacter* HitPlayer, const FVector& ImpactPoint) override;

public:
	// 프리즈(이동 불가) 지속 시간 (초). 이 시간이 끝나면 데미지가 들어간다.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|SlashFreeze",
		meta = (ClampMin = "0.0"))
	float FreezeDuration = 2.0f;

	// 프리즈 종료 = 지연 데미지가 실제로 들어가는 순간 재생할 사운드 (맞은 플레이어 위치, 모든 클라)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|SlashFreeze")
	USoundBase* DelayedDamageSound = nullptr;

	// 지연 데미지 사운드 감쇠 설정
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|SlashFreeze")
	USoundAttenuation* DelayedDamageSoundAttenuation = nullptr;
};
