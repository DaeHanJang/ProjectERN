// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Combat/Weapons/ERNWeaponBase.h"
#include "GAS/Abilities/CharacterSkill/ERNProjectileSpawnTypes.h"
#include "ERNRangedWeapon.generated.h"

class AERNProjectileBase;
class UNiagaraSystem;
class UArrowComponent;
class ACharacter;
class USkeletalMeshComponent;

/**
 * AERNRangedWeapon - 원거리 무기
 * 투사체 스폰은 AnimNotify_SpawnProjectile에서 호출
 */
UCLASS()
class PROJECTERN_API AERNRangedWeapon : public AERNWeaponBase
{
	GENERATED_BODY()

public:
	AERNRangedWeapon();
	
	// 투사체 데이터
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Ranged|Projectile")
	FERNProjectileSpawnData ProjectileSpawnData;
	
	// 머즐 포인트 - 에디터에서 위치/회전 직접 지정 (스태틱 메시는 소켓 미지원)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|Ranged")
	TObjectPtr<UArrowComponent> MuzzlePoint;

	// 레거시 호환용 (스켈레탈 메시 사용 시 소켓 이름 지정 가능)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Ranged")
	FName MuzzleSocketName = TEXT("MuzzleSocket");
	
	// 투사체 소환 함수
	UFUNCTION(BlueprintCallable, Category = "Weapon|Ranged")
	void SpawnProjectile(ACharacter* OwnerCharacter, USkeletalMeshComponent* SourceMesh);

	// 실제 투사체 스폰 담당
	UFUNCTION(BlueprintCallable, Category="Weapon|Ranged")
	void SpawnProjectileAtTransform(TSubclassOf<AERNProjectileBase> ProjectileClass,
	                                FVector SpawnLocation,
	                                FRotator SpawnRotation,
	                                UNiagaraSystem* MuzzleEffect = nullptr);

	// 머즐 이펙트 - 모든 클라이언트에서 재생
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayMuzzleEffect(FVector Location, FRotator Rotation, UNiagaraSystem* Effect);

private:
	// 기본 발사 위치 계산 함수
	bool GetDefaultProjectileSpawnTransform(FVector& OutLocation, FRotator& OutRotation) const;
};
