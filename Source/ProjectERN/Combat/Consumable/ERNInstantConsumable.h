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
	virtual void ApplyEffectPlayer(AProjectERNCharacter* PlayerCharacter) override;
	virtual void ApplyEffectMonster(AERNEnemyCharacter* EnemyCharacter) override;
	
private:
	void UpdateApplyDamageToMonster(AERNEnemyCharacter* EnemyCharacter, int32 Index);
	
	UFUNCTION(NetMulticast, Unreliable)
	void Multicast_PlayEffectAndSound();
	
private:
	// 몬스터 효과 종류 (즉발형, 지속형)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Consumable", meta=(AllowPrivateAccess="true", EditCondition="ApplyType == EApplyType::Monster"))
	EMonsterEffectType MonsterEffectType = EMonsterEffectType::None;
	
	// 데미지
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Consumable", meta=(AllowPrivateAccess="true", EditCondition="ApplyType == EApplyType::Monster"))
	float MonsterDamage = 0.0f;
	
	// 지속형 틱 시간
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Consumable", meta=(AllowPrivateAccess="true", EditCondition="ApplyType == EApplyType::Monster && MonsterEffectType == EMonsterEffectType::Duration"))
	float MonsterRate = 0.0f;
	
	// 지속 시간
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Consumable", meta=(AllowPrivateAccess="true", EditCondition="ApplyType == EApplyType::Monster && MonsterEffectType == EMonsterEffectType::Duration"))
	float MonsterApplyTime = 0.0f;
	
	// VFX
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Consumable|Effect", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UNiagaraSystem> Effect;
	
	// Sound
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Consumable|Sound", meta=(AllowPrivateAccess="true"))
	TObjectPtr<USoundBase> Sound;
	
	bool bHitMonsterDuration = false;
	TArray<FTimerDelegate> MonsterApplyTimerDelegates;
	TArray<FTimerHandle> MonsterApplyTimers;
	TArray<float> CurrentTimers;
};
