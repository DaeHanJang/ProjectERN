// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Character/Enemy/ERNEnemyCharacter.h"
#include "ERNBossCharacter.generated.h"

class ULevelSequence;

// 보스 페이즈 정보
USTRUCT(BlueprintType)
struct FBossPhaseInfo
{
	GENERATED_BODY()

	// 이 페이즈가 활성화되는 체력 비율 (0.0 ~ 1.0)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float HealthThreshold = 1.0f;

	// 이 페이즈에서 사용할 비헤이비어 트리
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	class UBehaviorTree* PhaseBehaviorTree;

	// 페이즈 전환 시 재생할 몽타주
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimMontage* PhaseTransitionMontage;

	// 페이즈 전환 중 슈퍼아머 적용 여부
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bSuperArmorDuringTransition = true;
};

/**
 * 보스 몬스터 캐릭터 클래스
 * - 페이즈 시스템
 * - 패턴 기반 공격
 * - 특수 연출 (등장, 페이즈 전환, 사망)
 */
UCLASS()
class PROJECTERN_API AERNBossCharacter : public AERNEnemyCharacter
{
	GENERATED_BODY()

public:
	AERNBossCharacter();

	virtual float TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser) override;

	// 현재 페이즈 인덱스
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Boss")
	int32 CurrentPhaseIndex = 0;

	// 페이즈 전환 중인지 여부
	UPROPERTY(BlueprintReadOnly, Replicated, Category = "Boss")
	bool bIsTransitioningPhase = false;

	// 페이즈 정보 배열
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Phases")
	TArray<FBossPhaseInfo> Phases;

	// 등장 연출 몽타주
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Cutscene")
	UAnimMontage* IntroMontage;

	// 사망 연출 몽타주
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Cutscene")
	UAnimMontage* DeathMontage;

	// 보스 이름 (UI 표시용)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|UI")
	FText BossName;

	// 페이즈 전환
	UFUNCTION(BlueprintCallable, Category = "Boss")
	void TransitionToPhase(int32 NewPhaseIndex);

	// 등장 연출 시작
	UFUNCTION(BlueprintCallable, Category = "Boss")
	void PlayIntro();

	// 현재 페이즈의 체력 비율 조건 체크
	UFUNCTION(BlueprintPure, Category = "Boss")
	int32 GetPhaseIndexForCurrentHealth() const;

protected:
	virtual void BeginPlay() override;
	virtual void OnDeath() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 페이즈 전환 완료 콜백
	UFUNCTION()
	void OnPhaseTransitionMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	// 슈퍼아머 적용/해제
	void ApplySuperArmor();
	void RemoveSuperArmor();

private:
	// 페이즈 전환 체크
	void CheckPhaseTransition();
};
