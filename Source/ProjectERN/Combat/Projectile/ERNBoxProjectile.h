// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Combat/Projectile/ERNProjectileBase.h"
#include "ERNBoxProjectile.generated.h"

class UBoxComponent;

/**
 * 
 */
UCLASS()
class PROJECTERN_API AERNBoxProjectile : public AERNProjectileBase
{
	GENERATED_BODY()

public:
	AERNBoxProjectile();
	
protected:
	virtual void BeginPlay() override;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Projectile|Collision")
	TObjectPtr<UBoxComponent> BoxCollision;
};
