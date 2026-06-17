// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "GameplayEffectTypes.h"
#include "GAS/Abilities/CharacterSkill/ERNGA_NormalSkillBase.h"
#include "ERNGA_Normal_PaladinShield.generated.h"

class AProjectERNCharacter;
class UAbilitySystemComponent;
class UGameplayEffect;
class USkeletalMeshComponent;

UCLASS()
class PROJECTERN_API UERNGA_Normal_PaladinShield : public UERNGA_NormalSkillBase
{
	GENERATED_BODY()

public:
	UERNGA_Normal_PaladinShield();

	// 스킬 적용 Notify
	void ApplyShieldFromNotify(USkeletalMeshComponent* MeshComp);
	
protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	                             const FGameplayAbilityActorInfo* ActorInfo,
	                             const FGameplayAbilityActivationInfo ActivationInfo,
	                             const FGameplayEventData* TriggerEventData) override;
	
	// Shield 값을 즉시 올려주는 Instant GE
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ERN|Skill")
	TSubclassOf<UGameplayEffect> ShieldAmountEffectClass;
	
	// 실드 지속시간과 SuperArmor 태그를 관리하는 Duration GE
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ERN|Skill")
	TSubclassOf<UGameplayEffect> ShieldStateEffectClass;
	
	// 적용 실드량 (시전자의 최대 체력 * ShieldAmountRate)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ERN|Skill", meta = (ClampMin = "0.0"))
	float ShieldAmountRate = 0.1f;

	// 실드 지속지간
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ERN|Skill", meta = (ClampMin = "0.0"))
	float ShieldDuration = 10.f;

	// 실드 적용 범위
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ERN|Skill", meta = (ClampMin = "0.0"))
	float ShieldRadius = 1500.f;
	
	// 시전자 적용 여부
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ERN|Skill|Shield")
	bool bIncludeSelf = true;
	
	// 기존 실드가 있으면 제거하고 새 실드로 교체할지 여부
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ERN|Skill|Shield")
	bool bReplaceExistingShield = true;
	
	// 디버그용
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ERN|Skill|Debug")
	bool bDrawDebugShieldRadius = false;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ERN|Skill|Debug", meta = (EditCondition = "bDrawDebugShieldRadius", ClampMin = "0.0"))
	float DebugShieldRadiusDuration = 2.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ERN|Skill|Debug", meta = (EditCondition = "bDrawDebugShieldRadius", ClampMin = "4"))
	int32 DebugShieldRadiusSegments = 48;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ERN|Skill|Debug", meta = (EditCondition = "bDrawDebugShieldRadius", ClampMin = "0.0"))
	float DebugShieldRadiusThickness = 2.0f;
	
private:
	// 중복 적용 방지
	bool bShieldAppliedThisActivation = false;

	UFUNCTION()
	void OnApplyShieldEventReceived(FGameplayEventData Payload);

	bool TryApplyShield(USkeletalMeshComponent* MeshComp);
	bool IsApplyShieldEventFromAvatar(const FGameplayEventData& Payload) const;
	USkeletalMeshComponent* GetAvatarMeshFromActorInfo() const;
	
	// 범위 내 아군을 찾아 실드를 적용
	void ApplyShieldToAllies(AProjectERNCharacter* Caster);

	// 단일 대상에게 실드 GE들을 적용
	void ApplyShieldToTarget(AProjectERNCharacter* Caster, AProjectERNCharacter* TargetCharacter, float CalculatedShieldAmount);
	
	// 적용할 실드 계산
	float CalculateShieldAmount(const AProjectERNCharacter* Caster) const;

	// 실드 적용 유효 대상 확인 (플레이어 캐릭터를 찾음) - 나중에 팀 시스템이 생기면 함수 수정 요구됨
	bool IsValidShieldTarget(const AProjectERNCharacter* Caster, const AProjectERNCharacter* TargetCharacter) const;

	// ShieldStateEffectClass가 제거될 때 남은 Shield 값을 정리
	void OnShieldStateEffectRemoved(const FGameplayEffectRemovalInfo& RemovalInfo);

	UFUNCTION()
	void OnMontageCompleted();

	UFUNCTION()
	void OnMontageBlendOut();

	UFUNCTION()
	void OnMontageInterrupted();

	UFUNCTION()
	void OnMontageCancelled();

	// GE 제거 콜백에서 어떤 ASC의 Shield를 정리할지 찾기 위한 맵
	TMap<FActiveGameplayEffectHandle, TWeakObjectPtr<UAbilitySystemComponent>> ActiveShieldStateEffectTargets;
};
