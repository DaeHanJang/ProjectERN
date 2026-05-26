// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ERNProjectileSpawnTypes.h"
#include "GAS/Abilities/CharacterSkill/ERNGA_NormalSkillBase.h"
#include "ERNGA_Normal_StarFall.generated.h"

class USkeletalMeshComponent;

UCLASS()
class PROJECTERN_API UERNGA_Normal_StarFall : public UERNGA_NormalSkillBase
{
	GENERATED_BODY()
	
public:
	UERNGA_Normal_StarFall();

	// Notify에서 호출할 함수 (투사체 발사 타이밍)
	void FireProjectileFromNotify(USkeletalMeshComponent* MeshComp);
	
protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	// 적용할 투사체 데이터
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ERN|Skill")
	FERNProjectileSpawnData ProjectileData;
	
private:
	UFUNCTION()
	void FinishSkill();
};
