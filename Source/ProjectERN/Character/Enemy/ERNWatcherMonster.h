// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Character/Enemy/ERNEnemyCharacter.h"
#include "ERNWatcherMonster.generated.h"

class USphereComponent;
class UDecalComponent;
class UStaticMeshComponent;

/**
 * 인스턴스 던전용 감시 몬스터 (데미지 0, 적발 시 특정 지점으로 순간이동)
 */
UCLASS()
class PROJECTERN_API AERNWatcherMonster : public AERNEnemyCharacter
{
	GENERATED_BODY()

public:
	AERNWatcherMonster();

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

protected:
	// --- 컴포넌트 ---
	
	// 감시 범위 전체를 덮는 투명한 구 콜리전 (플레이어 진입 감지용)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Surveillance")
	TObjectPtr<USphereComponent> SurveillanceCollision;

	// 바닥에 감시망을 그려주는 데칼 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Surveillance")
	TObjectPtr<UDecalComponent> SurveillanceDecal;

	// 허공에 입체적인 감시망 빛을 렌더링하는 메쉬 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Surveillance")
	TObjectPtr<UStaticMeshComponent> SurveillanceMesh;

	// --- 행동 패턴 설정 ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol")
	bool bShouldRotate = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol", meta = (EditCondition = "bShouldRotate"))
	float RotationSpeed = 45.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol")
	bool bShouldMoveLeftRight = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol", meta = (EditCondition = "bShouldMoveLeftRight"))
	float MovementDistance = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Patrol", meta = (EditCondition = "bShouldMoveLeftRight"))
	float MovementSpeed = 1.0f; // 사이클 속도 조절용 승수

	// --- 감시 영역 설정 ---

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surveillance")
	float SurveillanceRadius = 1000.0f;

	// 부채꼴 시야각 (전방 기준 좌우 각도, ex: 45 입력시 총 90도)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surveillance")
	float SurveillanceAngle = 45.0f;

	// 감시망이 켜지고 꺼지는 1회 사이클 시간 (초 단위)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surveillance")
	float SurveillanceCycleTime = 4.0f;

	// 스케일/투명도가 이 값(0.0~1.0) 이상일 때만 감시판정 On
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surveillance")
	float SurveillanceActiveThreshold = 0.1f;

	// --- 텔레포트 설정 ---

	// 적발 시 플레이어를 이동시킬 목적지 액터 클래스 (월드에 배치된 이 클래스를 찾아 텔레포트)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Surveillance")
	TSubclassOf<AActor> TeleportPointClass;

private:
	// 초기 위치 및 벡터 (좌우 이동 계산용)
	FVector InitialLocation;
	FVector InitialRightVector;
	
	// 블루프린트에서 세팅한 원본 스케일 기억용
	FVector InitialDecalScale;
	FVector InitialMeshScale;

	// 내부 시간 기록
	float CurrentCycleTime = 0.0f;

	// 감시 활성화 상태 (스케일에 따라 Tick에서 갱신됨)
	bool bIsSurveillanceActive = false;

	// 콜리전 내부에 있는 플레이어 추적용
	UPROPERTY()
	TArray<AActor*> OverlappingPlayers;

	UFUNCTION()
	void OnSurveillanceBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnSurveillanceEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	void TeleportPlayer(AActor* PlayerActor);
};
