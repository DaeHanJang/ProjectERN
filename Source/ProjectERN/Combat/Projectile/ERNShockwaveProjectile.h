// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Combat/Projectile/ERNProjectileBase.h"
#include "ERNShockwaveProjectile.generated.h"

class UStaticMeshComponent;
class UDecalComponent;
class UNiagaraComponent;
class UNiagaraSystem;
class UMaterialInterface;
class UMaterialInstanceDynamic;

/**
 * AERNShockwaveProjectile - 제자리에서 원형 충격파가 0→최대 반경으로 커지며
 * 지면을 휩쓰는 투사체. 확장하는 front(앞면)가 지나갈 때 한 번씩만 타격하며,
 * 그 순간 점프로 떠 있으면 회피 가능
 * - 데미지는 AERNProjectileBase의 Damage/StaggerPower/넉백 재사용
 * - VFX는 Niagara / Mesh / Decal 중 할당된 것에 CurrentRadius를 구동 (디버그 원 지원)
 */
UCLASS()
class PROJECTERN_API AERNShockwaveProjectile : public AERNProjectileBase
{
	GENERATED_BODY()

public:
	AERNShockwaveProjectile();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	// 확장 front가 이번 틱에 지나간 플레이어를 1회 타격
	void SweepDamage();

	// 할당된 VFX(Niagara/Mesh/Decal)에 현재 반경 반영
	void UpdateVisual();

	// === 확장 설정 ===
	// 최대 반경 (cm)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shockwave", meta = (ClampMin = "0.0"))
	float MaxRadius = 7000.f;

	// 확장 속도 = 휩쓰는 속도 (cm/s)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shockwave", meta = (ClampMin = "1.0"))
	float ExpandSpeed = 1200.f;

	// 이 높이 이하의 대상만 타격 (점프로 이보다 높이 뜨면 회피)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shockwave", meta = (ClampMin = "0.0"))
	float MaxHitHeight = 200.f;

	// 충격파 front(앞면)의 두께 (cm) — 이 띠 안에 있을 때만 타격
	// 크면 잘 안 빠짐(안정적), 작으면 회피 타이밍이 빡빡해짐
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shockwave", meta = (ClampMin = "1.0"))
	float FrontThickness = 150.f;

	// 스폰 시 지면으로 원점 스냅 (소켓 높이와 무관하게 원점 Z를 지면에 맞춤)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shockwave")
	bool bSnapToGround = true;

	// 지면 스냅 트레이스 길이 (위/아래)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shockwave", meta = (EditCondition = "bSnapToGround"))
	float GroundTraceHalfHeight = 500.f;

	// === VFX (할당된 것만 동작) ===
	// 1) Niagara 충격파 (User float 파라미터로 반경 구동)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shockwave|VFX")
	UNiagaraSystem* ShockwaveFX = nullptr;

	// 2) 링/디스크 메시 (반경에 맞춰 스케일)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shockwave|VFX")
	UStaticMeshComponent* RingMesh;

	// 스케일 1일 때 RingMesh의 반경 (cm) — 스케일 = CurrentRadius / 이 값
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shockwave|VFX", meta = (ClampMin = "1.0"))
	float RingMeshUnitRadius = 50.f;

	// 3) 지면 데칼 (머티리얼 스칼라 파라미터로 반경 구동)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shockwave|VFX")
	UMaterialInterface* RingDecalMaterial = nullptr;

	// 데칼 프로젝션 깊이 (cm)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shockwave|VFX", meta = (ClampMin = "1.0"))
	float DecalProjectionDepth = 200.f;

	// Niagara/Decal 머티리얼에 넘길 파라미터 이름 (링 띠 = 실제 히트밴드)
	// 외곽 반경 (cm)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shockwave|VFX")
	FName RadiusParamName = TEXT("Radius");

	// 띠 두께 (cm) — 안쪽 반경 = Radius - FrontThickness
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shockwave|VFX")
	FName ThicknessParamName = TEXT("FrontThickness");

	// 최대 반경 (cm) — 데칼 UV→cm 환산용 (데칼 박스 크기와 동일)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shockwave|VFX")
	FName MaxRadiusParamName = TEXT("MaxRadius");

	// === 디버그 ===
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shockwave|Debug")
	bool bDrawDebug = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shockwave|Debug", meta = (EditCondition = "bDrawDebug"))
	FColor DebugColor = FColor::Cyan;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Shockwave|Debug", meta = (EditCondition = "bDrawDebug", ClampMin = "4"))
	int32 DebugSegments = 48;

private:
	// 충격파 Niagara 컴포넌트 (ShockwaveFX 할당 시 사용)
	UPROPERTY()
	UNiagaraComponent* ShockwaveNiagara;

	// 지면 데칼 컴포넌트 (RingDecalMaterial 할당 시 사용)
	UPROPERTY()
	UDecalComponent* RingDecal;

	// 데칼 동적 머티리얼 인스턴스
	UPROPERTY()
	UMaterialInstanceDynamic* DecalMID;

	// 현재 반경
	float CurrentRadius = 0.f;

	// 이미 타격한 대상 (중복 방지)
	UPROPERTY()
	TSet<TObjectPtr<AActor>> HitActors;
};
