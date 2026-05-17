// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_EnemySpawnProjectile.generated.h"

/**
 * 적 전용 투사체 스폰 Notify
 * ProjectileTag와 일치하는 FEnemyProjectileConfig를 찾아 투사체 스폰
 */
UCLASS(DisplayName = "Enemy Spawn Projectile")
class PROJECTERN_API UAnimNotify_EnemySpawnProjectile : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override { return TEXT("Enemy Spawn Projectile"); }

	// ERNEnemyCharacter의 ProjectileConfigs에서 찾을 태그
	UPROPERTY(EditAnywhere, Category = "Projectile")
	FName ProjectileTag = NAME_None;

	// true: AIC가 감지중인 모든 플레이어 중 한 명을 랜덤 타겟으로 선택
	// false: 기존 FocusActor(블랙보드 주 어그로) 방향
	UPROPERTY(EditAnywhere, Category = "Projectile")
	bool bRandomTarget = false;
};
