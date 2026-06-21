// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GAS/Abilities/CharacterSkill/ERNGA_UltimateSkillBase.h"
#include "ERNGA_Ult_CrimsonJudgment.generated.h"

class AProjectERNCharacter;
class USkeletalMeshComponent;
class UNiagaraSystem;
class AERNMeleeWeapon;

UCLASS()
class PROJECTERN_API UERNGA_Ult_CrimsonJudgment : public UERNGA_UltimateSkillBase
{
	GENERATED_BODY()

public:
	UERNGA_Ult_CrimsonJudgment();

	// 궁극기 동안 사용할 검 트레일 (서버가 모든 클라에 멀티캐스트로 교체, 종료 시 복귀)
	UPROPERTY(EditDefaultsOnly, Category = "Ultimate|Trail")
	TObjectPtr<UNiagaraSystem> UltimateTrailEffect = nullptr;

protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility,
		bool bWasCancelled) override;

private:
	UFUNCTION()
	void FinishSkill();

	// 장착 근접 무기 가져오기
	AERNMeleeWeapon* GetEquippedMeleeWeapon() const;

	// 서버에서만: 트레일 교체/복귀를 모든 클라에 멀티캐스트
	void ServerSetUltimateTrail(UNiagaraSystem* Trail);
};
