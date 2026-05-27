#pragma once

#include "CoreMinimal.h"
#include "ERNWeaponSkillTypes.generated.h"

class UNiagaraSystem;

// 스킬 판정 위치를 정하는 Enum
UENUM(BlueprintType)
enum class EWeaponSkillAreaOriginMode : uint8
{
	CharacterOffset UMETA(DisplayName = "Character Offset"),
	MeshSocket UMETA(DisplayName = "Mesh Socket"),
	WeaponHitbox UMETA(DisplayName = "Weapon Hitbox")
};

// 투사체 소환 위치를 정하는 Enum
UENUM(BlueprintType)
enum class EWeaponSkillProjectileSpawnSource : uint8
{
	CharacterSocket UMETA(DisplayName = "Character Socket"),
	WeaponMuzzle UMETA(DisplayName = "Weapon Muzzle"),
	WeaponHitbox UMETA(DisplayName = "Weapon Hitbox"),
	CharacterForward UMETA(DisplayName = "Character Forward")
};

// 나이아가라 적용 트리거
UENUM(BlueprintType)
enum class EERNWeaponSkillInstantEffectTrigger : uint8
{
	// 범위 공격 (무기 휘두르는 구간) 공격
	AreaBegin UMETA(DisplayName = "Area Begin"),
	AreaEnd UMETA(DisplayName = "Area End"),
	// 투사체 발사 공격
	ProjectileFire UMETA(DisplayName = "Projectile Fire"),
	// 폭발(범위 즉시 적용) 공격
	Explosion UMETA(DisplayName = "Explosion")
};

// BP에서 설정할 나이아가라 데이터
USTRUCT(BlueprintType)
struct FERNWeaponSkillInstantNiagaraEffect
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Niagara")
	bool bUseEffect = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Niagara", meta=(EditCondition="bUseEffect", EditConditionHides))
	EERNWeaponSkillInstantEffectTrigger Trigger = EERNWeaponSkillInstantEffectTrigger::Explosion;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Niagara", meta=(EditCondition="bUseEffect", EditConditionHides))
	TObjectPtr<UNiagaraSystem> NiagaraSystem;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Niagara", meta=(EditCondition="bUseEffect", EditConditionHides))
	EWeaponSkillAreaOriginMode OriginMode = EWeaponSkillAreaOriginMode::CharacterOffset;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Niagara", meta=(EditCondition="bUseEffect && OriginMode == EWeaponSkillAreaOriginMode::MeshSocket", EditConditionHides))
	FName MeshSocketName = TEXT("hand_r");

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Niagara", meta=(EditCondition="bUseEffect", EditConditionHides))
	FVector LocationOffset = FVector::ZeroVector;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Niagara", meta=(EditCondition="bUseEffect", EditConditionHides))
	FRotator RotationOffset = FRotator::ZeroRotator;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Niagara", meta=(EditCondition="bUseEffect", EditConditionHides))
	FVector Scale = FVector::OneVector;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Niagara", meta=(EditCondition="bUseEffect", EditConditionHides, ClampMin="0.0"))
	float StartDelay = 0.f;
};

// Multicast로 클라들에게 보내는 런타임 데이터
USTRUCT(BlueprintType)
struct FERNWeaponSkillInstantNiagaraSpawnData
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<UNiagaraSystem> NiagaraSystem;

	UPROPERTY()
	FVector Location = FVector::ZeroVector;

	UPROPERTY()
	FRotator Rotation = FRotator::ZeroRotator;

	UPROPERTY()
	FVector Scale = FVector::OneVector;

	UPROPERTY()
	float StartDelay = 0.f;
};
