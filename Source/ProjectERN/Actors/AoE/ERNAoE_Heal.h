// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ERNAoEActor.h"
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

	// Tick마다 적용할 회복량
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="ERN|AoE|Heal", meta=(ClampMin="0.0"))
	float HealAmountPerTick = 20.f;
	
private:
	// 헬퍼 함수
	UAbilitySystemComponent* GetASCFromActor(AActor* Actor) const;
};
