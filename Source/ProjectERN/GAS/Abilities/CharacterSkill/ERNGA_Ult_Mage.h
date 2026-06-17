// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ERNProjectileSpawnTypes.h"
#include "GAS/Abilities/CharacterSkill/ERNGA_UltimateSkillBase.h"
#include "ERNGA_Ult_Mage.generated.h"

class USkeletalMeshComponent;

/**
 *
 */
UCLASS()
class PROJECTERN_API UERNGA_Ult_Mage : public UERNGA_UltimateSkillBase
{
	GENERATED_BODY()

public:
	UERNGA_Ult_Mage();

	// Notify에서 호출 — 투사체 발사 타이밍. SpawnLocationOffset은 노티파이가 지정한 추가 오프셋
	void FireProjectileFromNotify(USkeletalMeshComponent* MeshComp, const FVector& SpawnLocationOffset = FVector::ZeroVector);

protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	// 발사할 투사체 데이터 (BP에서 설정)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ERN|Skill")
	FERNProjectileSpawnData ProjectileData;

private:
	UFUNCTION()
	void FinishSkill();

};
