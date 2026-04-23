// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_UpdateMotionWarpTarget.generated.h"

/**
 * Motion Warping 타겟 위치를 업데이트하는 Notify
 * 몽타주 실행 중 플레이어가 이동해도 추적 가능
 */
UCLASS()
class PROJECTERN_API UAnimNotify_UpdateMotionWarpTarget : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

	// Warp Target 이름 (기본값: "AttackTarget")
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Motion Warping")
	FName WarpTargetName = TEXT("AttackTarget");
};
