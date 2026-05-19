// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ERNIntroBird.generated.h"

class USceneComponent;
class USkeletalMeshComponent;
class UNiagaraComponent;
class UCurveFloat;
class AProjectERNCharacter;

/**
 * 인트로 시퀀스용 새 액터.
 * 서버에서 스폰되어 직선 경로로 비행하며, HangPoint에 플레이어 캐릭터를 부착한다.
 * 플레이어가 매달림에서 해제되면 위로 상승 후 자동 Destroy.
 */
UCLASS()
class PROJECTERN_API AERNIntroBird : public AActor
{
	GENERATED_BODY()

public:
	AERNIntroBird();

	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 비행 시작 (서버 권한)
	void StartFlight();

	// 부착된 플레이어가 새에서 해제됐을 때 호출 (서버 권한) → 위로 상승 후 Destroy
	void OnPlayerReleased();

	// 캐릭터 부착 슬롯
	USceneComponent* GetHangPoint() const { return HangPoint; }

	// 부착된 플레이어 설정
	void SetAttachedPlayer(AProjectERNCharacter* Player) { AttachedPlayer = Player; }

protected:
	virtual void BeginPlay() override;

	// 새 스켈레탈 메시
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USkeletalMeshComponent> BirdMesh;

	// 캐릭터가 매달릴 슬롯 (BP에서 위치 조정)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USceneComponent> HangPoint;

	// 비행 중 재생할 나이아가라 (BP에서 에셋 할당, bAutoActivate=true)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UNiagaraComponent> FlightVFX;

	// === 비행 파라미터 ===

	// Forward 방향으로 비행할 거리 (cm)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Flight")
	float FlightDistance = 5000.f;

	// 시작 → 끝까지 걸리는 시간 (초)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Flight")
	float FlightDuration = 4.f;

	// 시간 정규화(0~1) → 진행도(0~1) 곡선. 비어 있으면 선형
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Flight")
	TObjectPtr<UCurveFloat> FlightCurve;

	// === 이탈 후 ===

	// 플레이어 이탈 후 상승 지속 시간 (이후 Destroy)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FlyAway")
	float FlyAwayDuration = 3.f;

	// 이탈 후 상승 방향 (기본 Up)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FlyAway")
	FVector FlyAwayDirection = FVector::UpVector;

	// 이탈 후 상승 속도 (cm/s)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "FlyAway")
	float FlyAwaySpeed = 800.f;

private:
	// 부착된 플레이어 (Replicated)
	UPROPERTY(Replicated)
	TObjectPtr<AProjectERNCharacter> AttachedPlayer;

	// 비행 시작 위치 (서버에서 StartFlight 시 캐싱)
	FVector StartLocation = FVector::ZeroVector;

	// 비행 방향 (서버에서 StartFlight 시 캐싱)
	FVector FlightDirection = FVector::ForwardVector;

	// 비행 경과 시간
	float ElapsedTime = 0.f;

	// 비행 중 여부 (서버에서만 사용)
	bool bIsFlying = false;

	// 이탈 후 상승 중 여부
	bool bIsFlyingAway = false;

	// FlyAway 시작 시점에 Destroy 예약용 타이머
	FTimerHandle FlyAwayTimerHandle;

	void DestroySelf();
};
