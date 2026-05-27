// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_Ultimate_Explosion.generated.h"

class UERNGA_UltimateSkillBase;

UCLASS()
class PROJECTERN_API UAnimNotify_Ultimate_Explosion : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp,
	                    UAnimSequenceBase* Animation,
	                    const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override
	{
		return TEXT("Ultimate Explosion");
	}

private:
	UERNGA_UltimateSkillBase* FindActiveUltimateSkill(AActor* OwnerActor) const;
};
