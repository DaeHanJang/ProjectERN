// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ERNPiercingProjectile.h"
#include "Combat/Projectile/ERNProjectileBase.h"
#include "Components/SphereComponent.h"
#include "ERNCapsuleProjectile.generated.h"

class UCapsuleComponent;

UCLASS()
class PROJECTERN_API AERNCapsuleProjectile : public AERNPiercingProjectile
{
	GENERATED_BODY()

public:
	AERNCapsuleProjectile();

protected:
	virtual void BeginPlay() override;
	
	virtual UPrimitiveComponent* GetProjectileCollisionComponent() const override;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Projectile|Collision")
	TObjectPtr<UCapsuleComponent> CapsuleCollision;
};
