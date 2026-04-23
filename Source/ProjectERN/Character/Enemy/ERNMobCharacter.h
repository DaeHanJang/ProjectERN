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
	float LeashDistance = 2000.0f;

	// 귀환 중인지 여부
	UPROPERTY(BlueprintReadOnly, Category = "AI")
	bool bIsReturning = false;

protected:
	virtual void BeginPlay() override;
};
