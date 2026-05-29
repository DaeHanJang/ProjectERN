// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Character/Enemy/ERNMobCharacter.h"
#include "ERNTrainingDummy.generated.h"

/**
 * 연습장용 허수아비
 * - AERNMobCharacter 상속 (순찰 전용 BT는 더미 BP의 AIControllerClass에서 지정)
 * - 무적: 사망하지 않음 (HP 0에서 멈춤) — OnDeath 무력화
 * - 일정 시간 피격이 없으면 MaxHealth까지 점진 회복
 */
UCLASS()
class PROJECTERN_API AERNTrainingDummy : public AERNMobCharacter
{
	GENERATED_BODY()

public:
	AERNTrainingDummy();

	// 몹 어그로/동료 전파는 건너뛰고(EnemyCharacter 경유) 회복 타이머만 리셋
	virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

protected:
	// 무적 — 사망 처리하지 않음 (의도적으로 Super 미호출)
	virtual void OnDeath() override;

	// 피격 후 회복 시작까지 대기 시간
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dummy|Regen")
	float RegenDelay = 5.f;

	// 초당 회복량
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dummy|Regen")
	float RegenPerSecond = 200.f;

	// 회복 틱 간격(초)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dummy|Regen", meta = (ClampMin = "0.01"))
	float RegenTickInterval = 0.1f;

private:
	FTimerHandle RegenDelayTimerHandle;
	FTimerHandle RegenTickTimerHandle;

	// RegenDelay 만료 시 회복 틱 시작
	void StartRegen();

	// MaxHealth까지 점진 회복, 도달 시 정지
	void RegenTick();
};
