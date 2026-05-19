// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Combat/Projectile/ERNProjectileBase.h"
#include "ERNCapsuleProjectile.generated.h"

class UCapsuleComponent;

UCLASS()
class PROJECTERN_API AERNCapsuleProjectile : public AERNProjectileBase
{
	GENERATED_BODY()
	
public:
	AERNCapsuleProjectile();

protected:
	virtual void BeginPlay() override;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Projectile|Collision")
	TObjectPtr<UCapsuleComponent> CapsuleCollision;
};
