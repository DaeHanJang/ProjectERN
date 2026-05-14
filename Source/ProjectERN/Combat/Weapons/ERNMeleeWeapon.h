// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Combat/Weapons/ERNWeaponBase.h"
#include "ERNMeleeWeapon.generated.h"

class UBoxComponent;
class UNiagaraSystem;

/**
 * AERNMeleeWeapon - 근접 무기
 * 히트박스를 AnimNotifyState에서 활성화/비활성화
 */
UCLASS()
class PROJECTERN_API AERNMeleeWeapon : public AERNWeaponBase
{
	GENERATED_BODY()

public:
	AERNMeleeWeapon();

	// 히트박스 활성화 (AnimNotifyState_MeleeHitbox에서 호출)
	UFUNCTION(BlueprintCallable, Category = "Weapon|Melee")
	void EnableHitbox();

	// 히트박스 비활성화 (AnimNotifyState_MeleeHitbox에서 호출)
	UFUNCTION(BlueprintCallable, Category = "Weapon|Melee")
	void DisableHitbox();

protected:
	// 무기 히트박스
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Weapon|Melee")
	UBoxComponent* HitboxComponent;

	// 한 번의 공격에서 중복 히트 방지
	UPROPERTY()
	TArray<AActor*> HitActors;


	UFUNCTION()
	void OnHitboxOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex,
		bool bFromSweep, const FHitResult& SweepResult);

	// 히트 이펙트 - 모든 클라이언트에서 재생
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayHitEffect(FVector Location, FRotator Rotation);

public:
	// 히트 시 나이아가라 이펙트
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Weapon|Melee")
	UNiagaraSystem* HitEffect;
	
	// 히트박스 Getter (무기 스킬에서 사용)
	UBoxComponent* GetHitboxComponent() const { return HitboxComponent; }
};
