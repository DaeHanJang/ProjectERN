// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "AnimNotifyState_EnemyMeleeHitbox.generated.h"

/**
 * 적 전용 근접 히트박스 NotifyState
 * HitboxTag와 일치하는 BoxComponent를 찾아 ON/OFF
 * BP에서 BoxComponent에 태그를 부여
 */
UCLASS(DisplayName = "Enemy Melee Hitbox")
class PROJECTERN_API UAnimNotifyState_EnemyMeleeHitbox : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override { return TEXT("Enemy Melee Hitbox"); }

	// 활성화할 BoxComponent의 태그 (BP마다 다르게 설정)
	UPROPERTY(EditAnywhere, Category = "Hitbox")
	FName HitboxTag = NAME_None;

	// 데미지 오버라이드 — true면 HitboxConfig의 Damage 대신 이 값 사용 (스테이트별 데미지 차등)
	UPROPERTY(EditAnywhere, Category = "Hitbox")
	bool bOverrideDamage = false;

	UPROPERTY(EditAnywhere, Category = "Hitbox", meta = (EditCondition = "bOverrideDamage", ClampMin = "0.0"))
	float DamageOverride = 0.f;

	// 경직력 오버라이드 — true면 HitboxConfig의 StaggerPower 대신 이 값 사용
	UPROPERTY(EditAnywhere, Category = "Hitbox")
	bool bOverrideStaggerPower = false;

	UPROPERTY(EditAnywhere, Category = "Hitbox", meta = (EditCondition = "bOverrideStaggerPower", ClampMin = "0.0"))
	float StaggerPowerOverride = 0.f;

private:
	void SetHitboxEnabled(USkeletalMeshComponent* MeshComp, bool bEnabled);
};
