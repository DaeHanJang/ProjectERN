// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Combat/Projectile/ERNProjectileBase.h"
#include "ERNEldenStar.generated.h"

class USceneComponent;
class USoundAttenuation;
class UNiagaraSystem;

/**
 * AERNEldenStar - 적 머리 위를 천천히 따라다니며 짧은 간격으로
 * 작은 유도 자식 투사체를 발사하는 캐리어 투사체
 * Duration 만료 또는 벽 충돌 시 자기 위치에서 폭발
 */
UCLASS()
class PROJECTERN_API AERNEldenStar : public AERNProjectileBase
{
	GENERATED_BODY()

public:
	AERNEldenStar();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;

	// 0.5초 간격 타겟 유효성 검사 및 재검색
	void OnRetargetTick();

	// FireInterval마다 자식 투사체 스폰
	void OnFireTick();

	// Duration 만료 - 자기 위치에서 폭발
	void OnDurationEnded();

	// 현재 위치 기준 가장 가까운 살아있는 적 검색
	AActor* FindNearestEnemy() const;

	// 자식 투사체용: 검색 반경(RetargetSearchRadius) 내 살아있는 적 중 랜덤 1마리 (없으면 nullptr)
	AActor* FindRandomEnemyInRange() const;

	// 현재 타겟이 무효/사망이면 새 타겟으로 교체
	void EnsureValidTarget();

	// 자식 투사체 발사 사운드/이펙트 모든 클라이언트 재생
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayFireEffect(FVector Location);

public:
	// 유도 타겟이 추적할 더미 (적 머리 위로 매 틱 위치 갱신)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EldenStar")
	USceneComponent* HomingProxy;

	// 발사할 자식 투사체 클래스 (작은 유도 투사체 BP)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "EldenStar")
	TSubclassOf<AERNProjectileBase> SubProjectileClass;

	// 자식 투사체 발사 간격 (초)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "EldenStar",
		meta = (ClampMin = "0.05"))
	float FireInterval = 0.25f;

	// EldenStar 지속시간
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "EldenStar",
		meta = (ClampMin = "0.1"))
	float Duration = 15.f;

	// BeginPlay 후 첫 자식 투사체 발사까지 대기
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "EldenStar",
		meta = (ClampMin = "0.0"))
	float InitialFireDelay = 0.5f;

	// 적 중심 기준 위쪽 오프셋
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "EldenStar")
	float HomingZOffset = 200.f;

	// 타겟 재검색 반경 (부모 EldenStar 자체 유도용)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "EldenStar",
		meta = (ClampMin = "0.0"))
	float RetargetSearchRadius = 10000.f;

	// 자식 투사체가 노릴 적을 모으는 반경 (부모와 별개)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "EldenStar",
		meta = (ClampMin = "0.0"))
	float ChildTargetSearchRadius = 2500.f;

	// 타겟 재검색 주기
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "EldenStar",
		meta = (ClampMin = "0.05"))
	float RetargetCheckInterval = 0.5f;

	// 자식 발사 시 재생 사운드
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EldenStar|FX")
	USoundBase* FireSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EldenStar|FX")
	USoundAttenuation* FireSoundAttenuation;

	// 자식 발사 시 재생 나이아가라
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "EldenStar|FX")
	UNiagaraSystem* FireEffect;

private:
	FTimerHandle RetargetTimerHandle;
	FTimerHandle FireTimerHandle;
	FTimerHandle DurationTimerHandle;
};
