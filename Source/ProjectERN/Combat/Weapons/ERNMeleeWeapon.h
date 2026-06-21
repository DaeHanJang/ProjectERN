// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Combat/Weapons/ERNWeaponBase.h"
#include "Combat/ERNSkillDamageTypes.h"
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
	virtual void BeginPlay() override;

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
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Melee")
	USoundBase* HitSound;

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

	// ===== 검날 연장 (AnimNotifyState_BladeExtend에서 호출) =====
	// TraceEnd를 바깥으로 밀어 판정 길이를 늘린다. ExtendRadius는 연장 중 스윕 반경.
	// GrowDuration > 0 이면 그 시간 동안 1배→Multiplier배로 점진 성장, 0 이면 즉시 적용.
	void BeginBladeExtend(float Multiplier, float GrowDuration, float ExtendRadius);
	void TickBladeExtend(float DeltaTime);
	void EndBladeExtend();

	// 연장 구간 동안 트레일 나이아가라를 NewTrail로 교체, 끝나면 기본 트레일로 복귀
	void BeginBladeTrailSwap(UNiagaraSystem* NewTrail);
	void EndBladeTrailSwap();

	// 서버가 모든 클라에 트레일 교체를 알림 (Trail=null이면 기본 복귀). 궁극기 등 안정된 시점에서 호출.
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_SetBladeTrail(UNiagaraSystem* Trail);

	// 연장 구간 동안 멜리 데미지 스펙을 Data로 교체, 끝나면 기본 스펙으로 복귀
	void BeginBladeDamageOverride(const FERNSkillDamageData& Data);
	void EndBladeDamageOverride();

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

	// 이전 프레임 검선(시작/끝). 매 틱 현재 count로 이전/현재를 함께 재샘플 → 점 개수 불일치(SKIP) 원천 제거
	FVector PreviousTraceStart = FVector::ZeroVector;
	FVector PreviousTraceEnd = FVector::ZeroVector;
	// 마지막 트레이스 틱 시각. 직전 틱과의 간격으로 "새 스윙"을 감지 (토글되는 Begin/End에 의존하지 않기 위함)
	float LastTraceTickTime = 0.f;

	TArray<FVector> GetCurrentTracePoints() const;
	// 주어진 검선(Start→End)을 현재 count로 샘플링
	TArray<FVector> SampleBladePoints(const FVector& Start, const FVector& End) const;
	void HandleTraceHit(const FHitResult& HitResult);

	// 소유 클라가 적중을 서버에 보고 → 서버가 데미지 적용 (판정은 클라의 정상 몽타주 기반)
	// bOverride: 연장 스테이트의 데미지 오버라이드 적용 여부 (클라가 자기 상태로 판단해 전달)
	UFUNCTION(Server, Reliable)
	void Server_ApplyMeleeHit(AActor* Target, bool bOverride);

	// ===== 검날 연장 상태 (per-actor, NotifyState 공유 객체에 저장하면 안 되므로 무기가 보관) =====
	// BeginPlay에서 캐시한 TraceEnd 기본 상대 위치 (복원 기준)
	FVector DefaultTraceEndRelLocation = FVector::ZeroVector;
	// 목표 배수
	float BladeTargetMultiplier = 1.f;
	// 점진 성장에 걸리는 시간 (0 = 즉시)
	float BladeGrowDuration = 0.f;
	// 성장 경과 시간
	float BladeElapsed = 0.f;
	// 현재 성장 중인지
	bool bBladeExtending = false;
	// 연장 중 스윕 반경 (NotifyState에서 전달받음)
	float BladeExtendRadius = 36.f;

	// 연장 구간 동안 사용할 임시 트레일 (nullptr이면 기본 TrailEffect 사용)
	UPROPERTY(Transient)
	TObjectPtr<UNiagaraSystem> ActiveTrailOverride = nullptr;

	// 연장 구간 동안 멜리 데미지 스펙을 교체할지
	bool bUseBladeDamageOverride = false;
	// 교체 시 사용할 데미지 스펙 (NotifyState에서 전달받음)
	FERNSkillDamageData BladeDamageOverride;

	// 주어진 배수로 TraceEnd 재배치 (TraceStart 고정)
	void ApplyBladeMultiplier(float Multiplier);

	// 트레일 시각효과 (모든 클라이언트에서 로컬 재생)
	void StartTrail();
	void UpdateTrail();
	void StopTrail();

	// 런타임 트레일 인스턴스 (TraceStart에 1회 부착 후 재사용)
	UPROPERTY(Transient)
	TObjectPtr<UNiagaraComponent> TrailComp;

	// 트레일이 논리적으로 켜져 있는지. Niagara IsActive()는 소프트 Deactivate 후에도
	// 잔류 입자 때문에 true를 반환하므로, 재활성 판단에는 이 플래그를 사용한다.
	bool bTrailRunning = false;
	
	// 디버그
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Trace|Debug")
	bool bDrawDebugTrace = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Trace|Debug", meta=(ClampMin="0.0"))
	float DebugTraceDuration = 0.1f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Weapon|Trace|Debug", meta=(ClampMin="0.0"))
	float DebugTraceThickness = 1.5f;
};
