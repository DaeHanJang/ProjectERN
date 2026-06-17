// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "Combat/ERNSkillDamageTypes.h"
#include "GAS/Abilities/CharacterSkill/ERNGA_SkillBase.h"
#include "ERNGA_UltimateSkillBase.generated.h"

class AProjectERNCharacter;
class USkeletalMeshComponent;

// GameplayCue 데이터
USTRUCT(BlueprintType)
struct FERNUltimateGameplayCueData
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GameplayCue")
	bool bUseCue = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GameplayCue",
		meta=(EditCondition="bUseCue", EditConditionHides))
	FGameplayTag CueTag;

	// 캐릭터 기준 위치 보정. 폭발 Cue에서는 폭발 위치 기준 보정으로 사용.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GameplayCue",
		meta=(EditCondition="bUseCue", EditConditionHides))
	FVector LocationOffset = FVector::ZeroVector;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GameplayCue",
		meta=(EditCondition="bUseCue", EditConditionHides))
	FRotator RotationOffset = FRotator::ZeroRotator;

	// GameplayCue BP에서 RawMagnitude로 받아 스케일처럼 사용 가능.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="GameplayCue",
		meta=(EditCondition="bUseCue", EditConditionHides, ClampMin="0.0"))
	float CueScale = 1.f;
};

// 폭발 데이터
USTRUCT(BlueprintType)
struct FERNUltimateExplosionData
{
	GENERATED_BODY()

	FERNUltimateExplosionData() : DamageData(50.f, 2.f, 2.f, 100.f, 3.f)
	{
	}
	
	// 하나의 궁극기 발동 중 폭발 Notify가 여러 번 들어와도 한 번만 적용할지 여부.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Explosion")
	bool bApplyOnlyOncePerActivation = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Explosion")
	FERNSkillDamageData DamageData;
	
	// 캐릭터 중심 폭발 반경.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Explosion", meta=(ClampMin="0.0"))
	float DamageRadius = 600.f;

	// 폭발 중심 오프셋. X=전방, Y=우측, Z=상단.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Explosion")
	FVector OriginOffset = FVector::ZeroVector;

	// 폭발에 적용할 GameplayCue
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Explosion|GameplayCue")
	FERNUltimateGameplayCueData ExplosionCueData;

	// 디버그 옵션
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Explosion|Debug")
	bool bDrawDebug = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Explosion|Debug", meta=(EditCondition="bDrawDebug", EditConditionHides, ClampMin="0.0"))
	float DebugDrawTime = 1.f;
};

UCLASS(Abstract)
class PROJECTERN_API UERNGA_UltimateSkillBase : public UERNGA_SkillBase
{
	GENERATED_BODY()

public:
	UERNGA_UltimateSkillBase();

	// 공용 AnimNotify가 호출하는 진입점.
	void TriggerExplosionFromNotify(USkeletalMeshComponent* MeshComp);

protected:
	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	// 시전 시 적용할 큐
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ERN|GameplayCue")
	FERNUltimateGameplayCueData CastCueData;

	// 폭발 사용 여부
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ERN|Explosion")
	bool bUseExplosion = false;

	// bUseExplosion이 true면 위 데이터가 BP로 표시
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ERN|Explosion", meta=(EditCondition="bUseExplosion", EditConditionHides))
	FERNUltimateExplosionData ExplosionData;

	// 스킬 사용 시 GC 실행
	void ExecuteCastGameplayCue();
	
	// 폭발 적용 시 GC 실행
	void ExecuteExplosionGameplayCue(AProjectERNCharacter* Caster, const FVector& ExplosionOrigin);
	
	// BP에서 설정한 GameplayCue 데이터 적용 함수
	void ExecuteUltimateGameplayCue(const FERNUltimateGameplayCueData& CueData,
									AProjectERNCharacter* Caster,
									const FVector& BaseLocation,
									const FRotator& BaseRotation) const;
	
private:
	bool bExplosionAppliedThisActivation = false;

	UFUNCTION()
	void OnExplosionEventReceived(FGameplayEventData Payload);

	bool TryTriggerExplosion(USkeletalMeshComponent* MeshComp);
	bool IsExplosionEventFromAvatar(const FGameplayEventData& Payload) const;
	USkeletalMeshComponent* GetAvatarMeshFromActorInfo() const;

	bool CanTriggerExplosionFromNotify(USkeletalMeshComponent* MeshComp, AProjectERNCharacter*& OutCaster) const;
	FVector GetExplosionOrigin(const AProjectERNCharacter* Caster) const;
	void ApplyExplosionDamage(AProjectERNCharacter* Caster, const FVector& Origin) const;
};
