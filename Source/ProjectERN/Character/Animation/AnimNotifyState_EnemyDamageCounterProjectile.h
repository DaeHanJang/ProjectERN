// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "AnimNotifyState_EnemyDamageCounterProjectile.generated.h"

class UNiagaraSystem;
class USoundBase;

/**
 * 구간(Begin~End) 동안 받은 누적 데미지가 임계치 이상이면
 * 감지중인 모든 플레이어에게 투사체를 발사하는 NotifyState (구간당 1회)
 * - 임계치 도달 시 차지 연출(나이아가라 + 사운드) 재생 후 FireDelay 뒤에 발사
 * - 지연 발사는 구간 종료와 무관하게 진행 (적이 죽은 경우에만 취소)
 * - 누적 카운터는 AERNEnemyCharacter가 보유 (NotifyState 객체는 여러 적이 공유하므로)
 * - 발사 로직은 UAnimNotify_EnemySpawnProjectilePerPlayer와 동일 (한 명당 투사체 1개, 각자 추적)
 */
UCLASS(DisplayName = "Enemy Damage Counter Projectile")
class PROJECTERN_API UAnimNotifyState_EnemyDamageCounterProjectile : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override { return TEXT("Enemy Damage Counter Projectile"); }

	// ERNEnemyCharacter의 ProjectileConfigs에서 찾을 태그
	UPROPERTY(EditAnywhere, Category = "Projectile")
	FName ProjectileTag = NAME_None;

	// 구간 내 누적 데미지가 이 값 이상이면 발사
	UPROPERTY(EditAnywhere, Category = "Projectile", meta = (ClampMin = "0.0"))
	float DamageThreshold = 100.f;

	// 임계치 도달 순간 스폰 소켓 위치에 재생할 나이아가라 (월드 위치 1회 재생)
	UPROPERTY(EditAnywhere, Category = "Charge FX")
	TObjectPtr<UNiagaraSystem> ChargeNiagara = nullptr;

	// 임계치 도달 순간 재생할 사운드
	UPROPERTY(EditAnywhere, Category = "Charge FX")
	TObjectPtr<USoundBase> ChargeSound = nullptr;

	// 차지 연출 후 발사까지 대기 시간 (초). 0이면 즉시 발사
	UPROPERTY(EditAnywhere, Category = "Charge FX", meta = (ClampMin = "0.0"))
	float FireDelay = 1.f;

private:
	// 임계치 도달 시 감지중인 플레이어마다 투사체 스폰 (서버 전용)
	void SpawnProjectilePerPlayer(class AERNEnemyCharacter* Enemy) const;
};
