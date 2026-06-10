// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Abilities/ERNGA_WallJump.h"

#include "AbilitySystemComponent.h"
#include "Character/Player/ProjectERNCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GAS/ERNGameplayTags.h"

UERNGA_WallJump::UERNGA_WallJump()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	FGameplayTagContainer AssetTags;
	AssetTags.AddTag(TAG_Ability_Movement_WallJump);
	SetAssetTags(AssetTags);

	ActivationBlockedTags.AddTag(TAG_State_Movement_WallJumpUsed);
	ActivationBlockedTags.AddTag(TAG_State_Movement_Landing);
}

bool UERNGA_WallJump::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                         const FGameplayAbilityActorInfo* ActorInfo,
                                         const FGameplayTagContainer* SourceTags,
                                         const FGameplayTagContainer* TargetTags,
                                         FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	const AProjectERNCharacter* Character = Cast<AProjectERNCharacter>(ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr);

	if (!Character || !Character->GetCharacterMovement())
	{
		return false;
	}

	// 공중 상태에서만 작동
	if (!Character->GetCharacterMovement()->IsFalling())
	{
		return false;
	}

	// 벽 점프를 이미 사용했다면 중복 사용 불가
	const UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	if (!ASC || ASC->HasMatchingGameplayTag(TAG_State_Movement_WallJumpUsed))
	{
		return false;
	}

	FHitResult WallHit;
	return FindWall(Character, WallHit);
}

void UERNGA_WallJump::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
                                      const FGameplayAbilityActorInfo* ActorInfo,
                                      const FGameplayAbilityActivationInfo ActivationInfo,
                                      const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	AProjectERNCharacter* Character =
		Cast<AProjectERNCharacter>(ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr);

	UAbilitySystemComponent* ASC =
		ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;

	if (!Character || !ASC)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	FHitResult WallHit;
	if (!FindWall(Character, WallHit))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	const FVector WallNormal = WallHit.ImpactNormal.GetSafeNormal();
	const FVector LaunchVelocity =
		(WallNormal * WallJumpHorizontalPower) +
		(FVector::UpVector * WallJumpVerticalPower);

	Character->LaunchCharacter(LaunchVelocity, true, true);

	ASC->SetLooseGameplayTagCount(TAG_State_Movement_WallJumpUsed, 1);
	
	Cast<AERNCharacterBase>(ActorInfo->AvatarActor.Get())->Multicast_PlayEffectAndSound(nullptr, FVector::ZeroVector, JumpSound, Character->GetActorLocation());

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

bool UERNGA_WallJump::FindWall(const ACharacter* Character, FHitResult& OutHit) const
{
	if (!Character || !Character->GetWorld())
	{
		return false;
	}

	const FVector Start = Character->GetActorLocation();

	const FVector Forward = Character->GetActorForwardVector();
	const FVector Right = Character->GetActorRightVector();

	const FVector Directions[] =
	{
		Forward,
		-Forward,
		Right,
		-Right
	};

	FCollisionQueryParams Params(SCENE_QUERY_STAT(WallJumpTrace), false);
	Params.AddIgnoredActor(Character);

	bool bFoundWall = false;
	float BestDistanceSq = TNumericLimits<float>::Max();

	for (const FVector& Direction : Directions)
	{
		const FVector End = Start + Direction.GetSafeNormal() * WallCheckDistance;

		FHitResult Hit;
		const bool bHit = Character->GetWorld()->SweepSingleByChannel(
			Hit,
			Start,
			End,
			FQuat::Identity,
			WallTraceChannel,
			FCollisionShape::MakeSphere(WallCheckRadius),
			Params);

		if (!bHit || !Hit.bBlockingHit)
		{
			continue;
		}

		const float DistanceSq = FVector::DistSquared(Start, Hit.ImpactPoint);
		if (DistanceSq < BestDistanceSq)
		{
			BestDistanceSq = DistanceSq;
			OutHit = Hit;
			bFoundWall = true;
		}
	}

	return bFoundWall;
}
