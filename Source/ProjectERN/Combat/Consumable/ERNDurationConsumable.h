// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Combat/Consumable/ERNConsumableBase.h"
#include "ERNDurationConsumable.generated.h"

class UNiagaraComponent;
/**
 * 
 */
UCLASS()
class PROJECTERN_API AERNDurationConsumable : public AERNConsumableBase
{
	GENERATED_BODY()
	
public:
	AERNDurationConsumable();
	
protected:
	virtual void BeginPlay() override;
	
	virtual void ApplyEffect() override;
	virtual void ApplyEffectPlayer(AProjectERNCharacter* PlayerCharacter) override;
	virtual void ApplyEffectMonster(AERNEnemyCharacter* EnemyCharacter) override;
	
private:
	void UpdateFogCollision();
	
	UFUNCTION()
	void OnFogBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	
	UFUNCTION()
	void OnFogEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
	
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayEffectAndSound();
private:
	// Fog Collision
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Collision", meta=(AllowPrivateAccess="true"))
	TObjectPtr<USphereComponent> FogCollision;
	
	// VFX Component
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Effect", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UNiagaraComponent> EffectComponent;
	
	// SFX Component
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Sound", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UAudioComponent> SoundComponent;
	
	// 틱 시간
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Consumable", meta=(AllowPrivateAccess="true"))
	float Rate = 0.0f;
	
	// 지속 시간
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Consumable", meta=(AllowPrivateAccess="true"))
	float ApplyTime = 0.0f;
	
	// 데미지
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Consumable", meta=(AllowPrivateAccess="true"))
	float MonsterDamage = 0.0f;
	
	FTimerHandle ApplyTimerHandle;
	float CurrentTime;
	
	UPROPERTY(Transient)
	TArray<AActor*> OverlappedActors;
	
};
