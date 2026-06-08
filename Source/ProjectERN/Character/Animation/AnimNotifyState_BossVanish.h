// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "AnimNotifyState_BossVanish.generated.h"

/**
 * 몽타주 구간 동안 보스를 "그 자리에 없는 것처럼" 만드는 NotifyState
 * - BGM 즉시 정지(재생 위치 보존) → 구간 끝나면 페이드 인 재개
 * - 무형 상태(충돌 끄기 + 락온 해제 + 유도 투사체 추적 중단)
 * - 몽타주가 Multicast로 모든 클라에서 재생되므로 각 클라 로컬에서 처리
 *   (BGM은 로컬, 무형 상태는 서버에서 설정 후 리플리케이트)
 */
UCLASS(DisplayName = "Boss Vanish")
class PROJECTERN_API UAnimNotifyState_BossVanish : public UAnimNotifyState
{
	GENERATED_BODY()

public:
	virtual void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration, const FAnimNotifyEventReference& EventReference) override;
	virtual void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override { return TEXT("Boss Vanish"); }

	// 재등장 시 BGM 페이드 인 시간 (초). 0이면 즉시 풀볼륨
	UPROPERTY(EditAnywhere, Category = "Vanish", meta = (ClampMin = "0.0"))
	float BGMFadeInTime = 0.5f;
};
