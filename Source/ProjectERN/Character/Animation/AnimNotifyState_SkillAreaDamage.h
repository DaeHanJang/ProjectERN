// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "AnimNotifyState_SkillAreaDamage.generated.h"

class UNiagaraComponent;
class UNiagaraSystem;

/**
 * 
 */
UENUM(BlueprintType)
enum class ESkillAreaDamageOriginMode : uint8
{
	CharacterOffset UMETA(DisplayName = "Character Offset"),
	MeshSocket UMETA(DisplayName = "Mesh Socket"),
	WeaponHitbox UMETA(DisplayName = "Weapon Hitbox")
};

UCLASS()
class PROJECTERN_API UAnimNotifyState_SkillAreaDamage : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp,
	                         UAnimSequenceBase* Animation,
	                         float TotalDuration,
	                         const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyTick(USkeletalMeshComponent* MeshComp,
	                        UAnimSequenceBase* Animation,
	                        float FrameDeltaTime,
	                        const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp,
	                       UAnimSequenceBase* Animation,
	                       const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override
	{
		return TEXT("Skill Area Damage");
	}

private:
	class UERNGA_WeaponSkill_Instant* FindActiveInstantWeaponSkill(AActor* OwnerActor) const;
};
