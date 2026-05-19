#pragma once

#include "CoreMinimal.h"
#include "ERNWeaponSkillTypes.generated.h"

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
