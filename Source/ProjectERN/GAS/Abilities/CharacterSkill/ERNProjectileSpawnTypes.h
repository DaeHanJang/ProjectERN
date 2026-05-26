#pragma once

#include "CoreMinimal.h"
#include "ERNProjectileSpawnTypes.generated.h"

class AERNProjectileBase;
class UNiagaraSystem;

UENUM(BlueprintType)
enum class EERNProjectileSpawnSource : uint8
{
	CharacterSocket UMETA(DisplayName = "Character Socket"),
	WeaponMuzzle UMETA(DisplayName = "Weapon Muzzle"),
	WeaponHitbox UMETA(DisplayName = "Weapon Hitbox"),
	CharacterForward UMETA(DisplayName = "Character Forward")
};

USTRUCT(BlueprintType)
struct FERNProjectileSpawnData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile")
	bool bUseProjectile = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile", meta = (EditCondition = "bUseProjectile", EditConditionHides))
	TSubclassOf<AERNProjectileBase> ProjectileClass;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile", meta = (EditCondition = "bUseProjectile", EditConditionHides))
	TObjectPtr<UNiagaraSystem> MuzzleEffect = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile", meta = (EditCondition = "bUseProjectile", EditConditionHides))
	EERNProjectileSpawnSource SpawnSource = EERNProjectileSpawnSource::WeaponMuzzle;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile", meta = (EditCondition = "bUseProjectile && SpawnSource == EERNProjectileSpawnSource::CharacterSocket", EditConditionHides))
	FName CharacterSocketName = TEXT("hand_r");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile", meta = (EditCondition = "bUseProjectile && SpawnSource == EERNProjectileSpawnSource::CharacterForward", EditConditionHides))
	float CharacterForwardDistance = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile", meta = (EditCondition = "bUseProjectile", EditConditionHides))
	FVector SpawnOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile", meta = (EditCondition = "bUseProjectile", EditConditionHides))
	bool bUseSourceRotation = false;
};