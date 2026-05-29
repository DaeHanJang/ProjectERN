// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Actors/AoE/ERNAoE_Heal.h"
#include "GAS/Abilities/CharacterSkill/ERNGA_UltimateSkillBase.h"
#include "ERNGA_Ult_Sanctuary.generated.h"

class AProjectERNCharacter;
class USkeletalMeshComponent;

UCLASS()
class PROJECTERN_API UERNGA_Ult_Sanctuary : public UERNGA_UltimateSkillBase
{
	GENERATED_BODY()

public:
	UERNGA_Ult_Sanctuary();

	void SpawnAoEFromNotify(USkeletalMeshComponent* MeshComp);

protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	// 실제 주기 힐을 처리할 AoE Actor BP.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ERN|Skill")
	TSubclassOf<AERNAoE_Heal> AoEActorClass;
	
	// 하나의 궁극기 발동 중 Notify가 여러 번 들어와도 AoE를 한 번만 생성할지 여부
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ERN|Skill")
	bool bSpawnAoEOnlyOncePerActivation = true;

	// 시전자 기준 AoE 생성 위치 보정. X=전방, Y=오른쪽, Z=위
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ERN|Skill")
	FVector AoEOriginOffset = FVector::ZeroVector;

	// AoE 생성 위치에 실행할 GameplayCue
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ERN|Skill|GameplayCue")
	FERNUltimateGameplayCueData AoECueData;

private:
	bool bAoESpawnedThisActivation = false;

	// 소환 가능한 캐릭터 존재 여부 확인
	bool CanSpawnAoEFromNotify(USkeletalMeshComponent* MeshComp, AProjectERNCharacter*& OutCaster) const;
	
	UFUNCTION()
	void FinishSkill();
};
