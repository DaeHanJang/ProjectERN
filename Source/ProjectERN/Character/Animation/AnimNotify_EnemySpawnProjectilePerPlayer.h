// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_EnemySpawnProjectilePerPlayer.generated.h"

/**
 * 감지중인 플레이어마다 투사체를 하나씩 스폰하는 Notify
 * - 플레이어 1~3명 변동에 동적으로 대응 (한 명당 투사체 1개, 각자 추적)
 * - ProjectileConfigs에서 ProjectileTag와 일치하는 투사체 클래스를 사용
 */
UCLASS(DisplayName = "Enemy Spawn Projectile Per Player")
class PROJECTERN_API UAnimNotify_EnemySpawnProjectilePerPlayer : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override { return TEXT("Enemy Spawn Projectile Per Player"); }

	// ERNEnemyCharacter의 ProjectileConfigs에서 찾을 태그
	UPROPERTY(EditAnywhere, Category = "Projectile")
	FName ProjectileTag = NAME_None;
};
