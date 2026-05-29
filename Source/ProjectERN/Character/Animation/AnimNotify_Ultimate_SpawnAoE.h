// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_Ultimate_SpawnAoE.generated.h"

class UERNGA_Ult_Sanctuary;

UCLASS()
class PROJECTERN_API UAnimNotify_Ultimate_SpawnAoE : public UAnimNotify
{
	GENERATED_BODY()
	
public:
	virtual void Notify(
		USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;
	virtual FString GetNotifyName_Implementation() const override
	{
		return TEXT("Ultimate Spawn AoE");
	}

private:
	UERNGA_Ult_Sanctuary* FindActiveSanctuarySkill(AActor* OwnerActor) const;
};
