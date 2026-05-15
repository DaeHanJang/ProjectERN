// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ERNGA_WeaponSkill_Instant.h"
#include "GAS/Abilities/ERNGA_WeaponSkill.h"
#include "ERNGA_WeaponSkill_Buff.generated.h"

class UGameplayEffect;
class UNiagaraSystem;

/*
USTRUCT(BlueprintType)
struct FERNWeaponSkillBuffCastEffectData
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="CastEffect")
	bool bUseCastEffect = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="CastEffect", meta=(EditCondition="bUseCastEffect", EditConditionHides))
	TObjectPtr<UNiagaraSystem> CastEffect;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="CastEffect", meta=(EditCondition="bUseCastEffect", EditConditionHides))
	EWeaponSkillAreaOriginMode OriginMode = EWeaponSkillAreaOriginMode::MeshSocket;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="CastEffect", meta=(EditCondition="bUseCastEffect && OriginMode == EWeaponSkillAreaOriginMode::MeshSocket", EditConditionHides))
	FName MeshSocketName = FName(TEXT("hand_r"));

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="CastEffect", meta=(EditCondition="bUseCastEffect", EditConditionHides))
	FVector LocationOffset = FVector::ZeroVector;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="CastEffect", meta=(EditCondition="bUseCastEffect", EditConditionHides))
	FRotator RotationOffset = FRotator::ZeroRotator;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="CastEffect", meta=(EditCondition="bUseCastEffect", EditConditionHides))
	FVector Scale = FVector::OneVector;
};
*/

// GameplayCue 방식으로 변경
USTRUCT(BlueprintType)
struct FERNWeaponSkillBuffCastCueData
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="CastCue")
	bool bUseCastCue = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="CastCue", meta=(EditCondition="bUseCastCue", EditConditionHides))
	FGameplayTag CastCueTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="CastCue", meta=(EditCondition="bUseCastCue", EditConditionHides))
	EWeaponSkillAreaOriginMode OriginMode = EWeaponSkillAreaOriginMode::MeshSocket;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="CastCue", meta=(EditCondition="bUseCastCue && OriginMode == EWeaponSkillAreaOriginMode::MeshSocket", EditConditionHides))
	FName MeshSocketName = FName(TEXT("hand_r"));

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="CastCue", meta=(EditCondition="bUseCastCue", EditConditionHides))
	FVector LocationOffset = FVector::ZeroVector;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="CastCue", meta=(EditCondition="bUseCastCue", EditConditionHides))
	FRotator RotationOffset = FRotator::ZeroRotator;

	// GameplayCue BP에서 RawMagnitude로 받아 Scale처럼 사용 가능
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="CastCue", meta=(EditCondition="bUseCastCue", EditConditionHides))
	float CueScale = 1.f;
};

UCLASS()
class PROJECTERN_API UERNGA_WeaponSkill_Buff : public UERNGA_WeaponSkill
{
	GENERATED_BODY()
	
public:
	UERNGA_WeaponSkill_Buff();

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	UFUNCTION(BlueprintCallable, Category="WeaponSkill|Buff")
	void ApplyBuffFromNotify(USkeletalMeshComponent* MeshComp);

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="WeaponSkill|Buff")
	TSubclassOf<UGameplayEffect> BuffEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="WeaponSkill|Buff", meta=(ClampMin="1.0"))
	float BuffEffectLevel = 1.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="WeaponSkill|Buff")
	bool bApplyBuffOnlyOncePerActivation = true;

	// UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="WeaponSkill|Buff|CastEffect")
	// FERNWeaponSkillBuffCastEffectData CastEffectData;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="WeaponSkill|Buff|CastCue")
	FERNWeaponSkillBuffCastCueData CastCueData;

private:
	bool bBuffAppliedThisActivation = false;

	// 스킬 사용 버프
	// void SpawnCastEffect(USkeletalMeshComponent* MeshComp) const;
	// bool GetCastEffectTransform(USkeletalMeshComponent* MeshComp, FTransform& OutTransform) const;
	void ExecuteCastGameplayCue(USkeletalMeshComponent* MeshComp) const;
	bool GetCastCueTransform(USkeletalMeshComponent* MeshComp, FTransform& OutTransform) const;
};
