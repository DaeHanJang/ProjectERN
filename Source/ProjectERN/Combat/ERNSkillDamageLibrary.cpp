#include "Combat/ERNSkillDamageLibrary.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Character/Enemy/ERNEnemyCharacter.h"
#include "Character/Player/ProjectERNCharacter.h"
#include "Combat/Weapons/ERNWeaponBase.h"
#include "Engine/DamageEvents.h"
#include "GAS/ERNAttributeSet.h"
#include "Inventory/Components/ERNEquipmentComponent.h"

float UERNSkillDamageLibrary::CalculateSkillDamage(AActor* SourceActor, const FERNSkillDamageData& DamageData)
{
	return DamageData.BaseDamage
		+ GetSourceAttackPower(SourceActor) * DamageData.AttackPowerScale
		+ GetSourceAttackPowerBonus(SourceActor)
		+ GetSourceWeaponDamage(SourceActor) * DamageData.WeaponDamageScale;
}

float UERNSkillDamageLibrary::GetSourceAttackPower(AActor* SourceActor)
{
	const UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(SourceActor);

	return ASC ? ASC->GetNumericAttribute(UERNAttributeSet::GetAttackPowerAttribute()) : 0.f;
}

float UERNSkillDamageLibrary::GetSourceAttackPowerBonus(AActor* SourceActor)
{
	const UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(SourceActor);

	return ASC ? ASC->GetNumericAttribute(UERNAttributeSet::GetAttackPowerBonusAttribute()) : 0.f;
}

float UERNSkillDamageLibrary::GetSourceWeaponDamage(AActor* SourceActor)
{
	if (!SourceActor)
	{
		return 0.f;
	}

	const UERNEquipmentComponent* Equipment = SourceActor->FindComponentByClass<UERNEquipmentComponent>();

	if (!Equipment || !Equipment->CurrentWeapon)
	{
		return 0.f;
	}

	return Equipment->CurrentWeapon->LightAttackDamage;
}

EERNSkillHitResult UERNSkillDamageLibrary::ApplySkillHit(AActor* TargetActor, AActor* SourceActor,
	AController* InstigatorController, const FERNSkillDamageData& DamageData, const FVector& HitOrigin)
{
	if (!TargetActor || !SourceActor || TargetActor == SourceActor)
	{
		return EERNSkillHitResult::None;
	}

	// 부활 공격 적용
	if (DamageData.bCanReviveDownedAlly && AProjectERNCharacter::TryApplyReviveHit(
			TargetActor,
			InstigatorController,
			DamageData.ReviveHitScale))
	{
		return EERNSkillHitResult::Revive;
	}
	
	// 적 확인
	AERNEnemyCharacter* Enemy = Cast<AERNEnemyCharacter>(TargetActor);
	if (!Enemy || Enemy->IsDead())
	{
		return EERNSkillHitResult::None;
	}

	// 스킬 대미지 계산
	const float DamageToApply = CalculateSkillDamage(SourceActor, DamageData);

	// 적에게 대미지 부여
	Enemy->TakeDamage(
		DamageToApply,
		FDamageEvent(),
		InstigatorController,
		SourceActor);

	// Stagger 부여
	if (DamageData.StaggerPower > 0.f)
	{
		Enemy->TryApplyStagger(DamageData.StaggerPower, HitOrigin);
	}

	return EERNSkillHitResult::Damage;
}
