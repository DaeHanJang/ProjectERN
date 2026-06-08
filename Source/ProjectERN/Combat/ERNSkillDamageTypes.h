#pragma once

#include "CoreMinimal.h"
#include "ERNSkillDamageTypes.generated.h"

USTRUCT(BlueprintType)
struct FERNSkillDamageData
{
	GENERATED_BODY()

	FERNSkillDamageData() = default;

	FERNSkillDamageData(
		float InBaseDamage,
		float InAttackPowerScale,
		float InWeaponDamageScale,
		float InStaggerPower = 0.f,
		float InReviveHitScale = 1.f)
		: BaseDamage(InBaseDamage)
		, AttackPowerScale(InAttackPowerScale)
		, WeaponDamageScale(InWeaponDamageScale)
		, StaggerPower(InStaggerPower)
		, ReviveHitScale(InReviveHitScale)
	{
	}

	// FinalDamage = BaseDamage + AttackPower * AttackPowerScale + AttackPowerBonus + WeaponDamage * WeaponDamageScale
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Damage", meta=(ClampMin="0.0"))
	float BaseDamage = 0.f;

	// 공격력 배율
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Damage", meta=(ClampMin="0.0"))
	float AttackPowerScale = 1.f;

	// 무기 대미지 배율
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Damage", meta=(ClampMin="0.0"))
	float WeaponDamageScale = 1.f;

	// 경직도 파워
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Stagger", meta=(ClampMin="0.0"))
	float StaggerPower = 0.f;

	// 아군 부활 대미지 적용 여부
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Revive")
	bool bCanReviveDownedAlly = true;

	// DownedComponent::ReviveHitAmount에 곱해질 배율. Ultimate는 이 값을 높이면 됨.
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Revive", meta=(ClampMin="0.0", EditCondition="bCanReviveDownedAlly", EditConditionHides))
	float ReviveHitScale = 1.f;
};
