// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "Combat/ERNSkillDamageTypes.h"
#include "AnimNotifyState_BladeExtend.generated.h"

class AERNMeleeWeapon;
class UNiagaraSystem;

/**
 * UAnimNotifyState_BladeExtend - 노티 구간 동안 검날(판정+트레일)을 늘렸다가 끝나면 복원.
 * MeleeTrace 스테이트와 같은(또는 살짝 넓은) 구간에 배치해 사용.
 */
UCLASS()
class PROJECTERN_API UAnimNotifyState_BladeExtend : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(
		USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		float TotalDuration,
		const FAnimNotifyEventReference& EventReference) override;

	virtual void NotifyTick(
		USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		float FrameDeltaTime,
		const FAnimNotifyEventReference& EventReference) override;

	virtual void NotifyEnd(
		USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override
	{
		return TEXT("Blade Extend");
	}

public:
	// 늘어날 배수 (기본 검 길이 대비)
	UPROPERTY(EditAnywhere, Category = "BladeExtend", meta = (ClampMin = "1.0"))
	float ExtendMultiplier = 3.0f;

	// 성장에 걸리는 시간(초). 0 = 즉시 스냅, > 0 = 그 시간 동안 점진 성장, < 0 = 노티 전체 길이에 맞춤
	UPROPERTY(EditAnywhere, Category = "BladeExtend")
	float GrowDuration = 0.0f;

	// 연장 중 스윕(히트 판정) 반경. 검날이 길어진 만큼 굵게.
	UPROPERTY(EditAnywhere, Category = "BladeExtend", meta = (ClampMin = "0.0"))
	float BladeExtendTraceRadius = 36.0f;

	// 연장 구간 동안 사용할 트레일 나이아가라 (비우면 트레일 교체 안 함)
	UPROPERTY(EditAnywhere, Category = "BladeExtend")
	TObjectPtr<UNiagaraSystem> ExtendTrailEffect = nullptr;

	// 연장 구간 동안 멜리 데미지 스펙을 교체할지
	UPROPERTY(EditAnywhere, Category = "BladeExtend|Damage")
	bool bOverrideDamage = false;

	// 연장 구간 동안 적용할 데미지 스펙 (BaseDamage / 공격력·무기 배율 / 경직 등 직접 편집)
	UPROPERTY(EditAnywhere, Category = "BladeExtend|Damage", meta = (EditCondition = "bOverrideDamage"))
	FERNSkillDamageData ExtendDamageData;

private:
	// Notify를 실행한 캐릭터가 장착한 근접 무기를 가져온다.
	AERNMeleeWeapon* GetEquippedMeleeWeapon(USkeletalMeshComponent* MeshComp) const;
};
