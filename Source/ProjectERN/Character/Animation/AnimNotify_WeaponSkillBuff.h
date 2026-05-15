// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_WeaponSkillBuff.generated.h"

class UERNGA_WeaponSkill_Buff;
/**
 * 
 */
UCLASS()
class PROJECTERN_API UAnimNotify_WeaponSkillBuff : public UAnimNotify
{
	GENERATED_BODY()
	
public:
	virtual void Notify(
		USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

private:
	UERNGA_WeaponSkill_Buff* FindActiveBuffWeaponSkill(AActor* OwnerActor) const;
};
