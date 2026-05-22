// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Combat/Consumable/ERNConsumableBase.h"
#include "ERNInstantConsumable.generated.h"

class AERNEnemyCharacter;
class AProjectERNCharacter;
class UNiagaraSystem;

UENUM(BlueprintType)
enum class EMonsterEffectType : uint8
{
	None     UMETA(Hidden),
	Instant  UMETA(DisplayName="Instant"),
	Duration UMETA(DisplayName="Duration")
};

/**
 * 
 */
UCLASS()
class PROJECTERN_API AERNInstantConsumable : public AERNConsumableBase
{
	GENERATED_BODY()
	
protected:
    virtual void ApplyEffect() override;
	
private:
	void ApplyEffectPlayer(AProjectERNCharacter* PlayerCharacter);
	void ApplyEffectMonster(AERNEnemyCharacter* EnemyCharacter);
	
	void UpdateApplyDamageToMonster(AERNEnemyCharacter* EnemyCharacter, int32 Index);
	
private:
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Consumable", meta=(AllowPrivateAccess="true"))
	EMonsterEffectType MonsterEffectType = EMonsterEffectType::None;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Consumable", meta=(AllowPrivateAccess="true"))
	float SweepRadius = 0.0f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Consumable", meta=(AllowPrivateAccess="true"))
	float MonsterDamage = 0.0f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Consumable", meta=(AllowPrivateAccess="true"))
	float MonsterRate = 0.0f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Consumable", meta=(AllowPrivateAccess="true"))
	float MonsterApplyTime = 0.0f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Consumable|Effect", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UNiagaraSystem> Effect;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Consumable|Sound", meta=(AllowPrivateAccess="true"))
	TObjectPtr<USoundBase> Sound;
	
	TArray<FTimerDelegate> MonsterApplyTimerDelegates;
	TArray<FTimerHandle> MonsterApplyTimers;
	TArray<float> CurrentTimers;
};
