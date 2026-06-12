// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Character/ERNCharacterBase.h"
#include "ERNEnemyCharacter.generated.h"

class UWidgetComponent;
class AERNProjectileBase;
class UBoxComponent;
class UMotionWarpingComponent;
class USoundBase;
class UNiagaraSystem;

USTRUCT(BlueprintType)
struct FEnemyHitboxConfig
{
	GENERATED_BODY()

	// BoxComponent의 ComponentTag와 일치시킬 태그
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName Tag;

	// 이 히트박스가 줄 데미지
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Damage = 10.f;

	// 이 히트박스가 줄 경직력
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float StaggerPower = 10.f;

	// 체력비례 추가 데미지 사용 여부 - true면 맞은 플레이어 최대HP의 일정 비율을 데미지에 합산
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bAddMaxHealthPercentDamage = false;

	// 추가 데미지 비율 - 맞은 플레이어 최대HP * 이 값을 데미지에 합산 (0.05 = 최대HP의 5%)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (EditCondition = "bAddMaxHealthPercentDamage", ClampMin = "0.0"))
	float MaxHealthPercentDamage = 0.05f;

	// 히트 시 재생할 사운드 (Multicast로 모든 머신에서 재생)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<USoundBase> HitSound;
};

USTRUCT(BlueprintType)
struct FEnemyProjectileConfig
{
	GENERATED_BODY()

	// AnimNotify에서 참조할 태그
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName Tag;

	// 스폰할 투사체 클래스
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<AERNProjectileBase> ProjectileClass;

	// 발사 소켓 이름 (스켈레탈 메시 소켓)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName SpawnSocket = TEXT("hand_r");
};

UCLASS(Abstract)
class PROJECTERN_API AERNEnemyCharacter : public AERNCharacterBase
{
	GENERATED_BODY()

public:
	AERNEnemyCharacter();

	virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	// 체력바 표시 (피격/락온 시 호출)
	UFUNCTION(NetMulticast, Reliable, BlueprintCallable, Category = "UI")
	void Multicast_ShowHealthBar();

	// 체력바 숨김
	UFUNCTION(NetMulticast, Reliable, BlueprintCallable, Category = "UI")
	void Multicast_HideHealthBar();

	// 공격 몽타주 재생 (모든 클라이언트에 동기화)
	UFUNCTION(NetMulticast, Reliable, BlueprintCallable, Category = "Combat")
	void Multicast_PlayAttackMontage(UAnimMontage* Montage);

	// 사망 직전 메시 숨김 (몽타주 종료 직전 T-pose 방지용)
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_HideOnDeath();

	// 히트 사운드를 모든 머신에서 재생 (Unreliable — 빈도 높을 수 있고 놓쳐도 OK)
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayHitSound(USoundBase* Sound, FVector Location);

	// 반격 차지 연출(나이아가라 + 사운드)을 모든 머신에서 재생 (Reliable — 발사 예고라 놓치면 안 됨)
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayCounterChargeFX(UNiagaraSystem* System, USoundBase* Sound, FVector Location);

protected:
	virtual void BeginPlay() override;

	// 사망 처리 오버라이드
	virtual void OnDeath() override;

	// 머리 위 체력바 위젯 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "UI")
	UWidgetComponent* HealthBarWidget;

	// 모션 워핑 컴포넌트 (공격 시 타겟 방향으로 이동)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Combat")
	UMotionWarpingComponent* MotionWarpingComponent;

	// 피격 후 체력바 자동 숨김까지 대기 시간 (블루프린트에서 수정 가능)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "UI")
	float HealthBarHideDelay = 15.0f;

	// 체력바 거리 기반 스케일링 - 이 거리에서 Scale 1.0
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|HealthBar")
	float HealthBarReferenceDistance = 2000.0f;

	// 멀어져도 이 값 이하로 안 작아짐
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|HealthBar", meta = (ClampMin = "0.01"))
	float HealthBarMinScale = 0.4f;

	// 가까워도 이 값 이상으로 안 커짐
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|HealthBar", meta = (ClampMin = "0.01"))
	float HealthBarMaxScale = 1.0f;

	// 이 거리 초과 시 체력바 숨김 (0 이하면 거리 제한 없음)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|HealthBar")
	float HealthBarMaxDisplayDistance = 4000.0f;

	// 체력바 스케일 갱신 주기 (초)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI|HealthBar", meta = (ClampMin = "0.01"))
	float HealthBarScaleUpdateInterval = 0.1f;

	// 사망 몽타주 종료 전 메시 숨김 시점 마진 (T-pose 방지용, 적마다 조정 가능)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Combat", meta = (ClampMin = "0.0"))
	float DeathCleanupLeadTime = 0.1f;

private:
	FTimerHandle HealthBarHideTimerHandle;
	FTimerHandle HealthBarScaleTimerHandle;

	// 체력바 표시 의도 - Show/Hide 호출로 토글, 거리 기반 숨김과 분리
	bool bHealthBarIntendedVisible = false;

	// Screen-space DrawSize는 픽셀 단위라 RelativeScale로 안 바뀜 - 초기값 캐싱 후 곱해서 적용
	FVector2D InitialHealthBarDrawSize = FVector2D(150.f, 20.f);

	// 카메라 거리에 따라 체력바 스케일 갱신 (로컬 - 각 클라이언트에서 실행)
	void UpdateHealthBarScale();

public:
	// 드롭 테이블
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drops")
	TObjectPtr<UDataTable> DropTable;

	// 사망 시 인스턴스 포탈 스폰 확률 (0~1, 몬스터별 설정). 0이면 스폰 안 함
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "InstancePortal", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float InstancePortalSpawnChance = 0.f;

	// 스폰할 인스턴스 포탈 클래스 (몬스터별로 다른 던전 포탈 지정 가능)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "InstancePortal")
	TSubclassOf<class AERNInstancePortal> InstancePortalClass;
	
	// 보상 골드
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drops")
	int32 BasicRewordGold = 1000;

	// 골드 편차
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Drops")
	int32 RewordGoldVariance = 10;

	// 초기 스탯 (BP마다 다르게 설정, BeginPlay에서 AttributeSet에 적용)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Stats")
	float InitialMaxHealth = 100.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Stats")
	float InitialMaxMana = 100.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Stats")
	float InitialMaxStamina = 100.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Stats")
	float InitialAttackPower = 10.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Stats")
	float InitialDefense = 5.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Stats")
	float InitialStaggerResistance = 10.f;

	// 전과(킬수/총데미지) 집계에서 제외 (연습용 허수아비 등)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Stats")
	bool bExcludeFromCombatStats = false;

	// 무적 - true면 모든 데미지 무시 (TakeDamage에서 즉시 0 반환). 런타임 토글 가능
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats")
	bool bIsImmortal = false;

	// 최종 데미지 랜덤 편차 (0.1 = ±10%). 0이면 편차 없음. Mob/Boss 공통 적용
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Stats", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DamageVariance = 0.1f;

	// 근접 히트박스 설정 (태그로 구분, 태그별 데미지 값 설정)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	TArray<FEnemyHitboxConfig> HitboxConfigs;

	// 원거리 투사체 설정 (태그로 구분)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Combat")
	TArray<FEnemyProjectileConfig> ProjectileConfigs;

	// 탐지 범위
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float DetectionRange = 1000.0f;

	// 공격 범위
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float AttackRange = 200.0f;

	// 순찰 속도
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float PatrolSpeed = 200.0f;

	// 추적 속도
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float ChaseSpeed = 450.0f;

	// 순찰 포인트 배열 (TargetPoint 액터들)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	TArray<AActor*> PatrolPoints;

	// MobSpawner가 스폰 직후 순찰 포인트 할당 (서버에서 호출)
	UFUNCTION(BlueprintCallable, Category = "AI")
	void SetPatrolPoints(const TArray<AActor*>& InPatrolPoints);

	// 한 번의 공격에서 중복 히트 방지
	TArray<AActor*> HitActors;

	// NotifyState End 시 호출
	void ClearHitActors() { HitActors.Empty(); }

	// 출력 데미지 배율 (기본 1.0). 동적 난이도에서 보스가 조우 시 설정. 잡몹은 1.0 유지.
	// 근접 히트박스/투사체 데미지에 곱해짐. 서버에서만 사용(데미지 계산이 서버 권위).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Combat", meta = (ClampMin = "0.0"))
	float OutgoingDamageMultiplier = 1.f;

	// AnimNotifyState가 활성화 구간 동안 설정하는 데미지/경직력 오버라이드 (HitboxConfig 값보다 우선)
	void SetHitboxOverride(bool bDamage, float Damage, bool bStagger, float StaggerPower, bool bMaxHealthPercent, float MaxHealthPercent);
	void ClearHitboxOverride();

	// NotifyState 구간 동안 받은 데미지 누적 추적 (임계치 도달 시 반격 투사체 발사용, 서버 전용)
	void StartDamageAccumulation();
	void StopDamageAccumulation();
	float GetAccumulatedDamage() const { return AccumulatedDamage; }

protected:
	// 드랍 처리
	UFUNCTION(BlueprintCallable, Category = "Enemy")
	virtual void SpawnDrops();

	// 골드 드롭
	UFUNCTION(BlueprintCallable, Category = "Enemy")
	virtual void SpawnGold();

	// 확률 판정 후 빈 도착 지점이 있으면 인스턴스 포탈 스폰 (서버 권위)
	void TrySpawnInstancePortal();

private:
	// NotifyState가 설정한 활성 오버라이드 (구간 종료 시 ClearHitboxOverride로 해제)
	bool  bActiveDamageOverride = false;
	float ActiveDamageOverride = 0.f;
	bool  bActiveStaggerOverride = false;
	float ActiveStaggerOverride = 0.f;
	bool  bActiveMaxHealthPercentOverride = false;
	float ActiveMaxHealthPercentOverride = 0.f;

	// NotifyState 구간 데미지 누적 (StartDamageAccumulation ~ StopDamageAccumulation 사이 TakeDamage에서 가산)
	bool  bTrackAccumulatedDamage = false;
	float AccumulatedDamage = 0.f;

	UFUNCTION()
	void OnHitboxOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	void BindHitboxOverlaps();
};
