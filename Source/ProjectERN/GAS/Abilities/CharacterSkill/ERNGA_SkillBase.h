// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayTagContainer.h"
#include "ERNGA_SkillBase.generated.h"

class AProjectERNCharacter;
class UAnimMontage;
class UAbilityTask_PlayMontageAndWait;
class UGameplayEffect;
class UNiagaraSystem;
class UTexture2D;

// 몽타주 섹션 구조체
USTRUCT(BlueprintType)
struct FERNSkillMontageSection
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Montage")
	FName SectionName = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Montage")
	float PlayRate = 1.0f;
};

/**
 * 스킬은 캐릭터마다 Normal/Ultimate가 한 개씩 부여되기 때문에 스킬마다가 아닌 Normal/Ultimate Slot별로 쿨타임을 적용.
 */
UCLASS(Abstract)
class PROJECTERN_API UERNGA_SkillBase : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UERNGA_SkillBase();

protected:
	// 스킬 모션 몽타주
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ERN|Skill|Montage")
	TObjectPtr<UAnimMontage> SkillMontage;

	// 몽타주 섹션 구조체
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ERN|Skill|Montage")
	TArray<FERNSkillMontageSection> MontageSections;

	// Ability가 종료되면 몽타주도 종료시킬것인지?
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ERN|Skill|Montage")
	bool bStopMontageWhenAbilityEnds = true;

	// 스킬 사용 후 적용할 쿨다운 GE
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ERN|Skill|Cooldown")
	TSubclassOf<UGameplayEffect> CooldownEffectClass;

	// BP에서 스킬마다 설정할 쿨타임
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ERN|Skill|Cooldown", meta=(ClampMin="0.0"))
	float CooldownDuration = 0.f;

	// 슬롯별 쿨다운 태그 (스킬별이 아님!)
	// 자식 클래스인 NormalSkillBase / UltimateSkillBase 생성자에서 자동 지정하므로 BP에서는 수정하지 않는다.(VisibleDefaultsOnly)
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category="ERN|Skill|Cooldown")
	FGameplayTagContainer CooldownTags;
	
	// 스킬 이미지
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="ERN|Skill|UI")
	TObjectPtr<UTexture2D> SkillIconTexture;

	// 쿨다운 확인
	virtual bool CheckCooldown(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	// 쿨다운 적용
	virtual void ApplyCooldown(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo) const override;

	// 몽타주 섹션 받기 헬퍼함수
	FName GetMontageSectionName(int32 Index) const;

	// 몽타주 실행
	UAbilityTask_PlayMontageAndWait* PlayConfiguredMontage(int32 SectionIndex = 0);

public:
	/** UI에서 사용하기 위한 함수로 public */

	// GAS가 이 어빌리티의 쿨다운 태그를 확인할 때 사용하는 함수
	virtual const FGameplayTagContainer* GetCooldownTags() const override;

	// UI에서 이 스킬 슬롯의 쿨다운 시간을 읽을 때 사용
	UFUNCTION(BlueprintPure, Category="ERN|Skill|Cooldown")
	float GetSkillCooldownDuration() const { return CooldownDuration; }

	// UI에서 이 스킬 슬롯의 쿨다운 태그를 읽을 때 사용
	UFUNCTION(BlueprintPure, Category="ERN|Skill|Cooldown")
	FGameplayTagContainer GetSkillCooldownTags() const { return CooldownTags; };

	// 쿨다운 적용에 필요한 값이 모두 준비됐는지 확인
	UFUNCTION(BlueprintPure, Category="ERN|Skill|Cooldown")
	bool IsSkillCooldownConfigured() const;
	
	// 스킬 이미지 Getter
	UFUNCTION(BlueprintPure, Category="ERN|Skill|UI")
	UTexture2D* GetSkillIconTexture() const { return SkillIconTexture; }
};
