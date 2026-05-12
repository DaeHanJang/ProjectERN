// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GAS/ERNGameplayAbility.h"
#include "ERNGA_WallJump.generated.h"

class ACharacter;
struct FHitResult;

/**
 * 
 */
UCLASS()
class PROJECTERN_API UERNGA_WallJump : public UERNGameplayAbility
{
	GENERATED_BODY()

public:
	UERNGA_WallJump();

	virtual bool CanActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr,
		const FGameplayTagContainer* TargetTags = nullptr,
		FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;

	virtual void ActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

protected:
	// 벽 찾는 거리
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ERN|WallJump")
	float WallCheckDistance = 90.f;
	
	// 벽 감지 판정 두께
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ERN|WallJump")
	float WallCheckRadius = 35.f;

	// 벽에서 밀려나는 힘
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ERN|WallJump")
	float WallJumpHorizontalPower = 650.f;

	// 위로 올라가는 힘
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ERN|WallJump")
	float WallJumpVerticalPower = 550.f;

	// 벽 충돌 채널
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ERN|WallJump")
	TEnumAsByte<ECollisionChannel> WallTraceChannel = ECC_Visibility;

	// 벽 찾는 함수
	bool FindWall(const ACharacter* Character, FHitResult& OutHit) const;
};
