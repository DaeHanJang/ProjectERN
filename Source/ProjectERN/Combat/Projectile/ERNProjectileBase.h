// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ERNProjectileBase.generated.h"

class USphereComponent;
class UProjectileMovementComponent;
class UNiagaraComponent;
class UNiagaraSystem;
class UStaticMeshComponent;
class UCurveFloat;
class USoundBase;
class USoundAttenuation;
class UAudioComponent;
class UCameraShakeBase;
class ACharacter;

/**
 * AERNProjectileBase - 원거리 투사체 베이스
 */
UCLASS()
class PROJECTERN_API AERNProjectileBase : public AActor
{
	GENERATED_BODY()

public:
	AERNProjectileBase();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	// 폰 외 충돌 처리 (벽 등) - WorldStatic/WorldDynamic Block 시 호출
	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	// 폰 오버랩 처리 - 같은 팀은 통과, 적팀은 데미지 적용
	UFUNCTION()
	void OnBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	// 유도 타겟 결정 및 PMC 세팅
	void InitializeHoming();

	// 플레이어 투사체용 자동 타겟 검색 (락온 미구현 시 임시)
	AActor* FindHomingTargetForPlayer() const;

	// 적 투사체용 블랙보드 TargetActor 조회
	AActor* GetEnemyBlackboardTarget() const;

	// 서버가 결정한 초기 방향이 클라에 도착했을 때 적용
	UFUNCTION()
	void OnRep_ChosenInitialDir();

	// 로컬 스페이스 방향을 받아 PMC Velocity / 액터 회전 적용
	void ApplyInitialDirection(const FVector& LocalDir);

	// 폭발 범위 데미지 적용
	void ApplyExplosionDamage(const FVector& ExplosionCenter);

	// 대상 캐릭터를 지정 방향으로 밀어냄 (서버 권위, Direction은 내부에서 정규화)
	void ApplyKnockback(ACharacter* TargetCharacter, const FVector& Direction, float Force);

	// 플레이어 owner 기준 직격/폭발 데미지 재계산 (서버, BeginPlay)
	void RecalculateDamageFromOwner();

	// 맞은 대상 최대HP 비례 추가 데미지 계산 (bAddMaxHealthPercentDamage true일 때만, 아니면 0)
	float GetMaxHealthBonusDamage(AActor* TargetActor) const;

public:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

public:
	// 충돌 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	USphereComponent* CollisionComponent;

	// 투사체 비주얼 메시 (화살, 마법구 등)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	UStaticMeshComponent* ProjectileMesh;

	// 이동 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	UProjectileMovementComponent* ProjectileMovement;

	// 비행 중 나이아가라 이펙트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	UNiagaraComponent* TrailEffect;

	// 착탄 시 나이아가라 이펙트
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Impact")
	UNiagaraSystem* ImpactEffect;

	// 착탄 나이아가라 이펙트 스케일
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Impact")
	FVector ImpactEffectScale = FVector::OneVector;
	
	// 착탄 시 사운드
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Impact")
	USoundBase* ImpactSound;

	// 사운드 감쇠 설정
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Impact")
	USoundAttenuation* ImpactSoundAttenuation;

	// 비행 중 루프 사운드
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	USoundBase* FlightSound;

	// 비행 사운드 감쇠 설정
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	USoundAttenuation* FlightSoundAttenuation;

	// 비행 사운드 컴포넌트
	UPROPERTY()
	UAudioComponent* FlightAudioComponent;

	// 소환 사운드
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Spawn")
	USoundBase* SpawnSound;

	// 소환 사운드 감쇠 설정
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Spawn")
	USoundAttenuation* SpawnSoundAttenuation;
	
	// 착탄 이펙트 - 모든 클라이언트에서 재생
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayImpactEffect(FVector Location, FRotator Rotation);

	// 지정 액터를 유도 타겟으로 가진 모든 투사체의 유도를 해제 (대상이 무형/사라질 때 호출)
	static void ClearHomingTargetingActor(UWorld* World, AActor* TargetActor);

	// 데미지 (플레이어 owner인 경우 BeginPlay에서 AttackPowerScale/WeaponDamageScale로 덮어씀)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile")
	float Damage = 20.0f;

	// 플레이어 공격력 계수 - 직격 데미지 = AttackPower * AttackPowerScale + WeaponDamage * WeaponDamageScale
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|Damage",
		meta = (ClampMin = "0.0"))
	float AttackPowerScale = 1.0f;

	// 무기 공격력 계수 - 직격 데미지 = AttackPower * AttackPowerScale + WeaponDamage * WeaponDamageScale
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|Damage",
		meta = (ClampMin = "0.0"))
	float WeaponDamageScale = 1.0f;

	// 체력비례 추가 데미지 사용 여부 - true면 직격/폭발 데미지에 맞은 대상 최대HP의 일정 비율을 합산
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|Damage")
	bool bAddMaxHealthPercentDamage = false;

	// 추가 데미지 비율 - 맞은 대상 최대HP * 이 값을 기본 데미지에 합산 (0.05 = 최대HP의 5%)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|Damage",
		meta = (EditCondition = "bAddMaxHealthPercentDamage", ClampMin = "0.0"))
	float MaxHealthPercentDamage = 0.05f;

	// 경직력 (BP_투사체마다 다르게 설정)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile")
	float StaggerPower = 15.f;

	// 넉백(밀어내기) 활성화 - 직격/폭발 양쪽에 적용 (BP_투사체별 개별 설정)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|Knockback")
	bool bKnockbackEnabled = false;

	// 직격 넉백 세기 (cm/s) - 맞은 대상을 투사체 진행 방향으로 밀어냄
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|Knockback",
		meta = (EditCondition = "bKnockbackEnabled", ClampMin = "0.0"))
	float KnockbackForce = 600.f;

	// 유도 활성화 (BP_투사체별 개별 설정)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|Homing")
	bool bHomingEnabled = false;

	// 유도력 - 클수록 타겟을 강하게 추적 (PMC HomingAccelerationMagnitude)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|Homing",
		meta = (EditCondition = "bHomingEnabled", ClampMin = "0.0"))
	float HomingAcceleration = 5000.f;

	// 플레이어 투사체 자동 타겟 검색 반경 (락온 미구현 임시)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|Homing",
		meta = (EditCondition = "bHomingEnabled", ClampMin = "0.0"))
	float HomingSearchRadius = 1500.f;

	// 적 탐색 전방 각도
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|Homing",
		meta = (EditCondition = "bHomingEnabled", ClampMin = "0.0", ClampMax = "180.0"))
	float HomingSearchHalfAngle = 60.f;

	// 시간에 따른 가속 활성화 (BP_투사체별 개별 설정)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|Acceleration")
	bool bAccelerateOverTime = false;

	// 초당 가속도 (cm/s²) - PMC의 MaxSpeed에서 클램프
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|Acceleration",
		meta = (EditCondition = "bAccelerateOverTime", ClampMin = "0.0"))
	float Acceleration = 500.f;

	// 시간(초) → 속도(cm/s) 매핑 곡선
	// 설정 시 매 Tick에서 곡선 값으로 속도 결정 (bAccelerateOverTime 무시)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|SpeedCurve")
	UCurveFloat* SpeedCurve = nullptr;

	// 커스텀 초기 발사 방향 사용 (BP_투사체별 개별 설정)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|InitialDirection")
	bool bUseCustomInitialDirection = false;

	// 랜덤 발사 방향 최소값 (로컬 스페이스, 자동 정규화)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|InitialDirection",
		meta = (EditCondition = "bUseCustomInitialDirection"))
	FVector MinInitialDirection = FVector::ForwardVector;

	// 랜덤 발사 방향 최대값 (로컬 스페이스, 자동 정규화)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|InitialDirection",
		meta = (EditCondition = "bUseCustomInitialDirection"))
	FVector MaxInitialDirection = FVector::ForwardVector;

	// 서버가 랜덤 결정한 발사 방향 - 클라에 InitialOnly로 동기화
	UPROPERTY(ReplicatedUsing = OnRep_ChosenInitialDir)
	FVector ChosenInitialDir = FVector::ZeroVector;
	
	// 유도 타켓 (서버-클라 리플리케이트)
	UPROPERTY(ReplicatedUsing = OnRep_HomingTarget)
	TWeakObjectPtr<class AActor> HomingTarget;
	
	UFUNCTION()
	void OnRep_HomingTarget();
	
	// PMC 유도 설정
	void ApplyHomingSettings(AActor* Target);

	// 폭발 활성화
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|Explosion")
	bool bExplode = false;

	// 폭발 범위 (cm)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|Explosion",
		meta = (EditCondition = "bExplode", ClampMin = "0.0"))
	float ExplosionRadius = 300.f;

	// 폭발 데미지 (플레이어 owner인 경우 BeginPlay에서 Explosion*Scale로 덮어씀)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|Explosion",
		meta = (EditCondition = "bExplode", ClampMin = "0.0"))
	float ExplosionDamage = 50.f;

	// 폭발 데미지용 플레이어 공격력 계수
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|Explosion",
		meta = (EditCondition = "bExplode", ClampMin = "0.0"))
	float ExplosionAttackPowerScale = 1.0f;

	// 폭발 데미지용 무기 공격력 계수
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|Explosion",
		meta = (EditCondition = "bExplode", ClampMin = "0.0"))
	float ExplosionWeaponDamageScale = 1.0f;

	// 폭발 경직력
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|Explosion",
		meta = (EditCondition = "bExplode", ClampMin = "0.0"))
	float ExplosionStaggerPower = 30.f;

	// 폭발 넉백 세기 (cm/s) - 폭발 중심에서 바깥(방사형)으로 밀어냄 (bKnockbackEnabled 활성화 시 작동)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|Explosion",
		meta = (EditCondition = "bExplode", ClampMin = "0.0"))
	float ExplosionKnockbackForce = 800.f;

	// 폭발 카메라 흔들림 (반경 내 모든 플레이어 카메라 거리 감쇠 흔들림)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|Explosion",
		meta = (EditCondition = "bExplode"))
	TSubclassOf<UCameraShakeBase> ExplosionShakeClass;

	// 폭발 카메라 흔들림 강도 배율 (거리 감쇠와 별개로 곱해짐)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|Explosion",
		meta = (EditCondition = "bExplode", ClampMin = "0.0"))
	float ExplosionShakeScale = 1.f;

	// 폭발 카메라 흔들림 풀 강도 반경 (cm)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|Explosion",
		meta = (EditCondition = "bExplode", ClampMin = "0.0"))
	float ExplosionShakeInnerRadius = 300.f;

	// 폭발 카메라 흔들림 0 강도 반경 (cm)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|Explosion",
		meta = (EditCondition = "bExplode", ClampMin = "0.0"))
	float ExplosionShakeOuterRadius = 1500.f;

	// 폭발 카메라 흔들림 감쇠 곡선
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile|Explosion",
		meta = (EditCondition = "bExplode", ClampMin = "0.0"))
	float ExplosionShakeFalloff = 1.f;

	// 폭발 카메라 흔들림 멀티캐스트 (모든 클라가 로컬에서 PlayWorldCameraShake 실행)
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayExplosionShake(FVector Origin);
};
