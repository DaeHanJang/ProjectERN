// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Combat/Projectile/ERNProjectileBase.h"
#include "ERNThunderCarrier.generated.h"

class USceneComponent;
class USoundAttenuation;
class UNiagaraSystem;

/**
 * AERNThunderCarrier - 할당된 플레이어 위를 천천히 따라다니며
 * 일정 간격으로 부모 위치에서 자식 투사체(벼락)를 아래로 떨어뜨리는 캐리어 투사체
 * - 타겟은 노티파이가 HomingTarget으로 주입 (플레이어 1명당 캐리어 1개)
 * - 자식의 속도/이펙트/데미지는 자식 BP에서 직접 설정
 */
UCLASS()
class PROJECTERN_API AERNThunderCarrier : public AERNProjectileBase
{
	GENERATED_BODY()

public:
	AERNThunderCarrier();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;

	// FireInterval마다 부모 위치에서 자식 투사체 스폰
	void OnFireTick();

	// Duration 만료 - 캐리어 소멸
	void OnDurationEnded();

	// 자가 복제 — ±SplitAngle 방향으로 클론 2개 생성 (세대 캡 적용)
	void DoSplit();

	// 타겟 미주입 시 폴백 (블랙보드 타겟 → 가장 가까운 플레이어)
	void EnsureValidTarget();

	// 가장 가까운 살아있는 플레이어 검색
	AActor* FindNearestPlayer() const;

	// 자식 스폰 사운드/이펙트 모든 클라이언트 재생
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayFireEffect(FVector Location);

public:
	// 타겟이 추적할 더미 (플레이어 위로 매 틱 위치 갱신)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Thunder")
	USceneComponent* HomingProxy;

	// true: 플레이어를 추적(유도) / false: 추적 없이 직진 (베이스 PMC가 처리)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Thunder")
	bool bFollowTarget = true;

	// === 분열 (자가 복제) ===
	// 분열 활성화 — SplitDelay 뒤 ±SplitAngle로 클론 2개 생성
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Thunder|Split")
	bool bSplitEnabled = false;

	// 스폰 후 분열까지 대기 (초)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Thunder|Split",
		meta = (EditCondition = "bSplitEnabled", ClampMin = "0.0"))
	float SplitDelay = 2.f;

	// 분열 각도 (좌우 각각, 도)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Thunder|Split",
		meta = (EditCondition = "bSplitEnabled", ClampMin = "0.0", ClampMax = "180.0"))
	float SplitAngle = 30.f;

	// 분열 세대 깊이 제한 (1 = 원본만 분열 → 총 3개, 2 = 7개 ...)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Thunder|Split",
		meta = (EditCondition = "bSplitEnabled", ClampMin = "0"))
	int32 MaxSplitGenerations = 1;

	// 이 캐리어의 분열 세대 (원본=0, 클론은 부모+1) — 런타임 주입
	UPROPERTY(BlueprintReadOnly, Category = "Thunder|Split")
	int32 SplitGeneration = 0;

	// 떨어뜨릴 자식 투사체(벼락) 클래스
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Thunder")
	TSubclassOf<AERNProjectileBase> StrikeProjectileClass;

	// 자식 투사체 발사 간격 (초)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Thunder",
		meta = (ClampMin = "0.05"))
	float FireInterval = 0.5f;

	// 캐리어 지속시간 (초)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Thunder",
		meta = (ClampMin = "0.1"))
	float Duration = 10.f;

	// BeginPlay 후 첫 발사까지 대기 (초)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Thunder",
		meta = (ClampMin = "0.0"))
	float InitialFireDelay = 0.5f;

	// 플레이어 중심 기준 위쪽 오프셋 (캐리어가 떠 있는 높이)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Thunder")
	float HomingZOffset = 600.f;

	// 타겟 폴백 검색 반경
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Thunder",
		meta = (ClampMin = "0.0"))
	float TargetSearchRadius = 10000.f;

	// 자식 발사 시 재생 사운드
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Thunder|FX")
	USoundBase* FireSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Thunder|FX")
	USoundAttenuation* FireSoundAttenuation;

	// 자식 발사 시 재생 나이아가라
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Thunder|FX")
	UNiagaraSystem* FireEffect;

private:
	FTimerHandle FireTimerHandle;
	FTimerHandle DurationTimerHandle;
	FTimerHandle SplitTimerHandle;
};
