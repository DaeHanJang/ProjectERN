// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Combat/Weapons/ERNWeaponBase.h"
#include "ERNMeleeWeapon.generated.h"

class UBoxComponent;
class UNiagaraSystem;
class UNiagaraComponent;
class UCameraShakeBase;

/**
 * AERNMeleeWeapon - 근접 무기
 * 히트박스를 AnimNotifyState에서 활성화/비활성화
 */
UCLASS()
class PROJECTERN_API AERNMeleeWeapon : public AERNWeaponBase
{
	GENERATED_BODY()

public:
	AERNMeleeWeapon();
	
	// 히트박스 활성화 (AnimNotifyState_MeleeHitbox에서 호출)
	UFUNCTION(BlueprintCallable, Category = "Weapon|Melee")
	void EnableHitbox();

	// 히트박스 비활성화 (AnimNotifyState_MeleeHitbox에서 호출)
	UFUNCTION(BlueprintCallable, Category = "Weapon|Melee")
	void DisableHitbox();
	
protected:
	// 무기 히트박스
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|Melee")
	UBoxComponent* HitboxComponent;

	// 한 번의 공격에서 중복 히트 방지
	UPROPERTY()
	TArray<AActor*> HitActors;


	UFUNCTION()
	void OnHitboxOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	// 히트 이펙트 - 모든 클라이언트에서 재생
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayHitEffect(FVector Location, FRotator Rotation);

public:
	// 히트 시 나이아가라 이펙트
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Melee")
	UNiagaraSystem* HitEffect;

	// 적 명중 시 공격자 카메라 흔들림 (Hit Confirmation)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Melee|CameraShake")
	TSubclassOf<UCameraShakeBase> HitConfirmShakeClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Melee|CameraShake", meta = (ClampMin = "0.0"))
	float HitConfirmShakeScale = 1.f;

	// 히트박스 Getter (무기 스킬에서 사용)
	UBoxComponent* GetHitboxComponent() const { return HitboxComponent; }
	
	// ===== 히트 판정 수정 - Trace 기반 =====
	void BeginAttackTrace();
	void TickAttackTrace();
	void EndAttackTrace();

	// ===== 검 트레일 =====
	// 무기별로 BP에서 할당하는 트레일 나이아가라
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Trail")
	TObjectPtr<UNiagaraSystem> TrailEffect;

	// 나이아가라에서 검 끝점을 받는 User 파라미터 이름
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Trail")
	FName TrailTipParamName = TEXT("SwordTip");
	
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon|Trace")
	TObjectPtr<USceneComponent> TraceStart;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Weapon|Trace")
	TObjectPtr<USceneComponent> TraceEnd;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Trace")
	float TraceRadius = 18.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Trace", meta=(ClampMin="2"))
	int32 TraceSampleCount = 5;

	bool bAttackTraceActive = false;

	TArray<FVector> PreviousTracePoints;

	TArray<FVector> GetCurrentTracePoints() const;
	void HandleTraceHit(const FHitResult& HitResult);

	// 트레일 시각효과 (모든 클라이언트에서 로컬 재생)
	void StartTrail();
	void UpdateTrail();
	void StopTrail();

	// 런타임 트레일 인스턴스 (TraceStart에 1회 부착 후 재사용)
	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> TrailComp;
	
	// 디버그
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Trace|Debug")
	bool bDrawDebugTrace = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Trace|Debug", meta=(ClampMin="0.0"))
	float DebugTraceDuration = 0.1f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Trace|Debug", meta=(ClampMin="0.0"))
	float DebugTraceThickness = 1.5f;
};
