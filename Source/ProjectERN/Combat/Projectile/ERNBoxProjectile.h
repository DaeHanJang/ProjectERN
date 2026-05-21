// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ERNPiercingProjectile.h"
#include "Combat/Projectile/ERNProjectileBase.h"
#include "ERNBoxProjectile.generated.h"

class UBoxComponent;

UCLASS()
class PROJECTERN_API AERNBoxProjectile : public AERNPiercingProjectile
{
	GENERATED_BODY()

public:
	AERNBoxProjectile();
	
protected:
	virtual void BeginPlay() override;
	
	virtual UPrimitiveComponent* GetProjectileCollisionComponent() const override;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Projectile|Collision")
	TObjectPtr<UBoxComponent> BoxCollision;
};
