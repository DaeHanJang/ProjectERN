// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Character/Enemy/ERNEnemyCharacter.h"
#include "ERNBossCharacter.generated.h"

class ULevelSequence;
class UBehaviorTree;
class UAnimMontage;
class USoundBase;
class UNiagaraSystem;

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
	UBehaviorTree* PhaseBehaviorTree = nullptr;

	// 페이즈 전환 시 재생할 몽타주
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UAnimMontage* PhaseTransitionMontage = nullptr;

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

	// 등장 연출 컷신 (시퀀서)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Cutscene")
	TSoftObjectPtr<ULevelSequence> IntroCutscene;

	// 등장 연출 몽타주 (컷신 없을 때 사용)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Cutscene")
	UAnimMontage* IntroMontage = nullptr;

	// 보스 이름 (UI 표시용)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|UI")
	FText BossName;

	// 최종보스 여부 — 이 보스를 잡으면 게임 클리어(승리) 처리
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss")
	bool bIsFinalBoss = false;

	// 중간보스 여부 — true면 사망 후 죽은 자리에 보스 포탈 스폰
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Boss")
	bool bIsMidBoss = false;

	// 스폰할 보스 포탈 클래스 (BP에서 지정)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Portal")
	TSubclassOf<class AERNBossPortal> BossPortalClass;

	// 사망 후 보스 포탈 스폰까지 딜레이(초)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Portal", meta = (ClampMin = "0.0"))
	float BossPortalSpawnDelay = 10.f;

	// 보스전 BGM (감지 시 재생)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Sound")
	TObjectPtr<USoundBase> BossBGM;

	// BGM 페이드 인 시간 (초)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Sound", meta = (ClampMin = "0.0"))
	float BGMFadeInTime = 1.0f;

	// BGM 페이드 아웃 시간 (사망 시)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|Sound", meta = (ClampMin = "0.0"))
	float BGMFadeOutTime = 2.0f;

	// BGM 시작됐는지 여부 (중복 발사 방지)
	UPROPERTY(BlueprintReadOnly, Category = "Boss|Sound")
	bool bBGMStarted = false;

	// 페이즈 전환
	UFUNCTION(BlueprintCallable, Category = "Boss")
	void TransitionToPhase(int32 NewPhaseIndex);

	// 등장 연출 시작
	UFUNCTION(BlueprintCallable, Category = "Boss")
	void PlayIntro();

	// 보스 잠금 (BT + Perception 동시 정지). 잠금 중에는 Perception 콜백이 안 불려서 체력바 트리거 X
	// — GameState가 보스 조우 컷신 흐름 중에 호출
	UFUNCTION(BlueprintCallable, Category = "Boss|Encounter")
	void SetIntroCutsceneLocked(bool bLocked);

	// 현재 잠금 상태 (디버그/조회용)
	UPROPERTY(BlueprintReadOnly, Category = "Boss|Encounter")
	bool bIsIntroLocked = false;

	// 현재 페이즈의 체력 비율 조건 체크
	UFUNCTION(BlueprintPure, Category = "Boss")
	int32 GetPhaseIndexForCurrentHealth() const;

	// 모든 플레이어에게 보스 체력바 표시 (AIC에서 호출)
	UFUNCTION(BlueprintCallable, Category = "Boss|UI")
	void ShowHealthBarToAllPlayers();

	// === 동적 난이도 ===
	// 파티 평균 공격력/레벨에 따라 보스 체력/출력 데미지를 조정 (조우 시 1회, 체력바 표시 직전 호출)
	UFUNCTION(BlueprintCallable, Category = "Boss|Difficulty")
	void ApplyDynamicDifficulty();

	// 체력 배율 계수 — 파티 평균 공격력이 기준치를 1 넘을 때마다 체력 +이 값(비율)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Difficulty", meta = (ClampMin = "0.0"))
	float HealthScalePerAttack = 0.01f;

	// 공격력 배율 계수 — 파티 평균 레벨이 기준치를 1 넘을 때마다 출력 데미지 +이 값(비율)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Difficulty", meta = (ClampMin = "0.0"))
	float AttackScalePerLevel = 0.2f;

	// 체력 스케일 기준치 (파티 평균 공격력)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Difficulty")
	float AttackBaseline = 100.f;

	// 공격 스케일 기준치 (파티 평균 레벨)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Difficulty")
	float LevelBaseline = 9.f;

	// 풀파티 인원수 — 이 인원이면 체력 100%. 인원이 적을수록 Count/FullPartySize 비율로 체력 감소 (예: 3명 기준 2명=0.66, 1명=0.33)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Difficulty", meta = (ClampMin = "1"))
	int32 FullPartySize = 3;

	// 체력/공격 배율 상한 (과도한 스케일 방지)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Difficulty", meta = (ClampMin = "1.0"))
	float MaxHealthMultiplier = 3.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Difficulty", meta = (ClampMin = "1.0"))
	float MaxAttackMultiplier = 2.f;

	// 인원 부족 시 공격 배율 감소량 — 풀파티에서 1명 모자랄 때마다 이 값만큼 공격 배율 감소 (예: 0.1 → 3명=1.0, 2명=0.9, 1명=0.8)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Difficulty", meta = (ClampMin = "0.0"))
	float AttackReductionPerMissingPlayer = 0.1f;

	// 인원 부족에 따른 공격 배율 하한 (너무 약해지지 않도록 제한)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Difficulty", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MinAttackPartyScale = 0.8f;

	// 난이도 스케일링(레벨/인원 배율)과 별개로 보스 최종 출력 데미지에 곱하는 전체 배율 (1.0=기본, 1.3=30% 증가)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Boss|Difficulty", meta = (ClampMin = "0.0"))
	float FinalDamageMultiplier = 1.f;

	// 모든 머신에서 보스 BGM 재생 (AIC가 첫 감지 시 호출)
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayBossBGM();

	// 모든 머신에서 보스 BGM 정지 (사망 시 호출)
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_StopBossBGM();

	// === 순간이동 번개 잔상 ===
	// 시작점~도착점을 잇는 번개 빔 Niagara (BeamStart/BeamEnd User 파라미터 필요)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Boss|VFX")
	TObjectPtr<UNiagaraSystem> TeleportTrailFX = nullptr;

	// 순간이동 시 모든 머신에서 시작→도착 번개 빔 스폰 (BTTask가 서버에서 호출)
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayTeleportTrail(FVector StartLocation, FVector EndLocation);

protected:
	virtual void BeginPlay() override;
	virtual void OnDeath() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// 페이즈 전환 완료 콜백
	UFUNCTION()
	void OnPhaseTransitionMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	void PlayPhaseTransitionMontage();

	// 페이즈 전환 몽타주 클라이언트 동기화
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlayPhaseTransitionMontage(UAnimMontage* Montage);

	// 인트로 컷신 종료 콜백
	UFUNCTION()
	void OnIntroCutsceneFinished();

	// 슈퍼아머 적용/해제
	void ApplySuperArmor();
	void RemoveSuperArmor();

private:
	// 페이즈 전환 체크
	void CheckPhaseTransition();

	// 체력바가 이미 표시되었는지 여부
	bool bHealthBarShown = false;

	// 동적 난이도가 이미 적용되었는지 여부 (중복 적용 방지)
	bool bDynamicDifficultyApplied = false;
};
