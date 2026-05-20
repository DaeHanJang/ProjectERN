// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Combat/Projectile/ERNProjectileBase.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "ERNPiercingProjectile.generated.h"

UCLASS()
class PROJECTERN_API AERNPiercingProjectile : public AERNProjectileBase
{
	GENERATED_BODY()

public:
	AERNPiercingProjectile();

protected:
	virtual void BeginPlay() override;
	virtual UPrimitiveComponent* GetProjectileCollisionComponent() const { return CollisionComponent; }
	void DisableProjectileCollision();

	// 관통 수 제한 없음 여부
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Projectile|Pierce")
	bool bUnlimitedPierce = false;

	// 최대 관통 수
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Projectile|Pierce",
		meta=(EditCondition="!bUnlimitedPierce", ClampMin="1"))
	int32 MaxPierceCount = 3;

	// 매 피격에 ImpactEffect 사용 여부
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Projectile|Pierce")
	bool bPlayImpactEffectOnEachHit = true;

	// Overlap 로직
	UFUNCTION()
	void OnPierceOverlap(UPrimitiveComponent* OverlappedComp,
	                     AActor* OtherActor,
	                     UPrimitiveComponent* OtherComp,
	                     int32 OtherBodyIndex,
	                     bool bFromSweep,
	                     const FHitResult& SweepResult);

private:
	// 피격당한 Actor 배열 (Set으로 중복 제거)
	TSet<TWeakObjectPtr<AActor>> PiercedActors;

	// 관통 횟수
	int32 CurrentPierceCount = 0;

	// 투사체 종료 여부
	bool bPierceFinished = false;

	// 투사체 삭제 로직
	void FinishPiercingProjectile();
};
