// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_Skill_SpawnProjectile.generated.h"

class UERNGA_Normal_StarFall;

UCLASS()
class PROJECTERN_API UAnimNotify_Skill_SpawnProjectile : public UAnimNotify
{
	GENERATED_BODY()
	
public:
	virtual void Notify(
		USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override
	{
		return TEXT("Skill Spawn Projectile");
	}

private:
	TObjectPtr<UERNGA_Normal_StarFall> FindActiveStarFallSkill(AActor* OwnerActor) const;
};
