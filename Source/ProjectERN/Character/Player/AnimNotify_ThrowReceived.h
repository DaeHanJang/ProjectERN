// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_ThrowReceived.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTERN_API UAnimNotify_ThrowReceived : public UAnimNotify
{
	GENERATED_BODY()
	
public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;
	
private:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tag", meta=(AllowPrivateAccess="true"))
	FGameplayTag EventTag;
	
};
