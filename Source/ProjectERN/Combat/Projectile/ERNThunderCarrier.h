// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Combat/Projectile/ERNProjectileBase.h"
#include "Combat/ERNSkillDamageTypes.h"
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

	// 소유자 팀 기준 가장 가까운 살아있는 적대 대상 검색
	// (적이 소유 → 플레이어 / 플레이어가 소유 → 적)
	AActor* FindNearestTarget() const;

	// 자식 스폰 사운드/이펙트 모든 클라이언트 재생
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayFireEffect(FVector Location);

	// 히트스캔 임팩트(번개) 이펙트/사운드 모든 클라이언트 재생 (충돌 지점)
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayStrikeImpact(FVector Location);

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

	// 떨어뜨릴 자식 투사체(벼락) 클래스.
	// 할당되면 기존 액터 스폰 방식, 비어 있으면 아래 히트스캔 폭발 방식 사용 (두 방식 비교용)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Thunder")
	TSubclassOf<AERNProjectileBase> StrikeProjectileClass;

	// === 히트스캔 폭발 (StrikeProjectileClass가 비어 있을 때 사용) ===
	// 아래로 트레이스할 최대 거리 (충돌 지점이 폭발 중심)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Thunder|Hitscan", meta = (ClampMin = "0.0"))
	float StrikeTraceDistance = 2000.f;

	// true면 정직하게 아래(0,0,-1) 대신 아래 범위에서 랜덤 방향(월드 스페이스)으로 트레이스 → 바닥 착탄 지점이 약간씩 흩어짐
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Thunder|Hitscan")
	bool bRandomizeStrikeDirection = false;

	// 랜덤 트레이스 방향 범위(월드, 컴포넌트별 RandRange 후 정규화). 기본값은 아래쪽으로 약간 퍼짐
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Thunder|Hitscan",
		meta = (EditCondition = "bRandomizeStrikeDirection"))
	FVector MinStrikeDirection = FVector(-0.3f, -0.3f, -1.f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Thunder|Hitscan",
		meta = (EditCondition = "bRandomizeStrikeDirection"))
	FVector MaxStrikeDirection = FVector(0.3f, 0.3f, -1.f);

	// 폭발 반경
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Thunder|Hitscan", meta = (ClampMin = "0.0"))
	float StrikeExplosionRadius = 200.f;

	// 폭발 데미지 (보스 OutgoingDamageMultiplier 곱해짐). bUseSkillDamageForStrike=false일 때 사용
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Thunder|Hitscan", meta = (ClampMin = "0.0"))
	float StrikeExplosionDamage = 0.f;

	// 플레이어 시전 시: StrikeExplosionDamage 대신 공격력/무기 데미지 스케일(SkillDamageData)로 데미지 계산
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Thunder|Hitscan")
	bool bUseSkillDamageForStrike = false;

	// 스킬 데미지 = BaseDamage + 공격력*배율 + 공격력보너스 + 무기데미지*배율. bUseSkillDamageForStrike일 때 사용
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Thunder|Hitscan",
		meta = (EditCondition = "bUseSkillDamageForStrike", EditConditionHides))
	FERNSkillDamageData StrikeSkillDamage;

	// 폭발 경직력
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Thunder|Hitscan", meta = (ClampMin = "0.0"))
	float StrikeStaggerPower = 10.f;

	// 체력비례 추가 데미지 사용 여부 (맞은 플레이어 최대HP 비율 합산)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Thunder|Hitscan")
	bool bStrikeAddMaxHealthPercentDamage = false;

	// 체력비례 데미지 비율 (0.05 = 최대HP의 5%)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Thunder|Hitscan",
		meta = (EditCondition = "bStrikeAddMaxHealthPercentDamage", ClampMin = "0.0"))
	float StrikeMaxHealthPercentDamage = 0.1f;

	// 폭발 넉백 사용 여부
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Thunder|Hitscan")
	bool bStrikeKnockback = false;

	// 넉백 세기
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Thunder|Hitscan",
		meta = (EditCondition = "bStrikeKnockback", ClampMin = "0.0"))
	float StrikeKnockbackForce = 800.f;

	// 임팩트(번개) 나이아가라 — 충돌 지점에 재생
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Thunder|Hitscan|FX")
	UNiagaraSystem* StrikeImpactEffect;

	// 임팩트 이펙트 강제 정리 시간 (초) — 루핑/무한 Lifetime 이펙트가 안 사라지고 누적되는 것 방지
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Thunder|Hitscan|FX", meta = (ClampMin = "0.1"))
	float StrikeImpactLifetime = 2.f;

	// 임팩트 사운드
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Thunder|Hitscan|FX")
	USoundBase* StrikeImpactSound;

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
