// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ERNAoEActor.h"
#include "Combat/ERNSkillDamageTypes.h"
#include "ERNAoE_Heal.generated.h"

class UGameplayEffect;
class UAbilitySystemComponent;

UCLASS()
class PROJECTERN_API AERNAoE_Heal : public AERNAoEActor
{
	GENERATED_BODY()
	
protected:
	// 유효한 대상인지 판단
	virtual bool IsValidAoETarget(AActor* TargetActor, UPrimitiveComponent* OverlappedComponent) const override;
	// 실제 효과 적용 로직을 구현하는 함수
	virtual void ApplyAoEToTarget(AActor* TargetActor) override;

	// 이 BP에서 적용한 회복량을 GE에 전달하기 위한 태그
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ERN|AoE|Heal")
	TSubclassOf<UGameplayEffect> HealEffectClass;

	// Tick마다 시전자 최대 체력 대비 적용할 회복 비율 (0.05 = 최대 체력의 5%)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ERN|AoE|Heal", meta=(ClampMin="0.0", ClampMax="1.0"))
	float HealPercentPerTick = 0.05f;

	// 범위 내 적에게 Tick마다 데미지를 줄지 여부
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ERN|AoE|Damage")
	bool bDamageEnemiesPerTick = true;

	// Tick마다 적에게 적용할 데미지 (시전자 공격력/무기 비례 + 경직). BaseDamage/Scale/StaggerPower는 BP에서 튜닝
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ERN|AoE|Damage", meta=(EditCondition="bDamageEnemiesPerTick"))
	FERNSkillDamageData DamagePerTick;

private:
	// 헬퍼 함수
	UAbilitySystemComponent* GetASCFromActor(AActor* Actor) const;
};
