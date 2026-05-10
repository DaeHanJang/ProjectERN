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
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile")
	UNiagaraSystem* ImpactEffect;

	// 착탄 이펙트 - 모든 클라이언트에서 재생
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayImpactEffect(FVector Location, FRotator Rotation);

	// 데미지 (무기의 AttackDamage가 여기로 전달됨)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile")
	float Damage = 20.0f;

	// 경직력 (BP_투사체마다 다르게 설정)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Projectile")
	float StaggerPower = 15.f;

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
};
