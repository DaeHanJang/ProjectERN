// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ERNConsumableBase.generated.h"

class AERNEnemyCharacter;
class AProjectERNCharacter;
class UGameplayEffect;
class UProjectileMovementComponent;
class UConsumableItemDataAsset;
class USphereComponent;

UENUM(BlueprintType)
enum class EApplyType : uint8
{
	None    UMETA(Hidden),
	Player  UMETA(DisplayName="Player"),
	Monster UMETA(DisplayName="Monster"),
	All     UMETA(DisplayName="All")
};

UCLASS()
class PROJECTERN_API AERNConsumableBase : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AERNConsumableBase();
	
	void Launch(const FVector& Direction);
	
protected:
	virtual void BeginPlay() override;
	
	UFUNCTION(BlueprintCallable, Category="Hit")
	void OnCollisionHit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
	
	UFUNCTION(BlueprintCallable, Category="Hit")
	virtual void ApplyEffect();
	
	UFUNCTION(BlueprintCallable, Category="Hit")
	virtual void ApplyEffectPlayer(AProjectERNCharacter* PlayerCharacter);
	
	UFUNCTION(BlueprintCallable, Category="Hit")
	virtual void ApplyEffectMonster(AERNEnemyCharacter* EnemyCharacter);
	
protected:
	// Collision
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Collision", meta=(AllowPrivateAccess="true"))
	TObjectPtr<USphereComponent> Collision;
	
	// Mesh
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mesh", meta=(AllowPrivateAccess="ture"))
	TObjectPtr<UStaticMeshComponent> Mesh;
	
	// MovementComponent
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mesh", meta=(AllowPrivateAccess="ture"))
	TObjectPtr<UProjectileMovementComponent> MovementComponent;
	
	// GE
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="GameplayAbility", meta=(AllowPrivateAccess="true"))
	TSubclassOf<UGameplayEffect> GE = nullptr;
	
	// 효과 적용 대상
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="GameplayAbility", meta=(AllowPrivateAccess="true"))
	EApplyType ApplyType = EApplyType::None;
	
	// 스윕 범위
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Consumable", meta=(AllowPrivateAccess="true"))
	float SweepRadius = 0.0f;
	
	bool bHit = false;
	
};
