// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Character/Enemy/ERNEnemyCharacter.h"
#include "ERNMobCharacter.generated.h"

/**
 * 일반 몬스터 캐릭터 클래스
 * - 단순 AI 로직 (시야 감지/피격 어그로 → 추적 → 공격)
 * - 귀환 로직
 */
UCLASS()
class PROJECTERN_API AERNMobCharacter : public AERNEnemyCharacter
{
	GENERATED_BODY()

public:
	AERNMobCharacter();

	// 피격 시 공격자를 타겟으로 설정
	virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	// 스폰 위치 (귀환용)
	UPROPERTY(BlueprintReadOnly, Category = "AI")
	FVector SpawnLocation;

	// 귀환 거리 (이 거리 이상 떨어지면 귀환)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI")
	float LeashDistance = 4000.0f;

	// 귀환 중인지 여부
	UPROPERTY(BlueprintReadOnly, Category = "AI")
	bool bIsReturning = false;

	// 피격 시 주변 동료 몬스터에게 어그로 전파 활성화
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Alert")
	bool bAlertNearbyMobs = true;

	// 어그로 전파 반경 (cm)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Alert",
		meta = (EditCondition = "bAlertNearbyMobs", ClampMin = "0.0"))
	float AlertRadius = 5000.0f;

	// 같은 클래스의 몹만 깨우기 (false면 모든 일반 몹)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AI|Alert",
		meta = (EditCondition = "bAlertNearbyMobs"))
	bool bAlertOnlySameType = false;

	// 풀파티 인원수 — 이 인원이면 체력 100%. 인원이 적을수록 Count/FullPartySize 비율로 체력 감소
	// (예: 3명 기준 2명=0.66, 1명=0.33)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Difficulty",
		meta = (ClampMin = "1"))
	int32 FullPartySize = 3;

protected:
	virtual void BeginPlay() override;

	// 파티 인원수에 따라 MaxHealth/Health 조절 (서버 권위)
	void ScaleHealthByPlayerCount();

	// 반경 내 다른 일반 몹들의 어그로를 Attacker로 잡아줌
	void AlertNearbyMobs(AActor* Attacker);
};
