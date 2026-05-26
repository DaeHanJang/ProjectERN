// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_Skill_ApplyBuff.generated.h"

class UERNGA_Normal_PaladinShield;

UCLASS()
class PROJECTERN_API UAnimNotify_Skill_ApplyBuff : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(
		USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override
	{
		return TEXT("Apply Buff");
	}

private:
	UERNGA_Normal_PaladinShield* FindActivePaladinShieldSkill(AActor* OwnerActor) const;
	
};
