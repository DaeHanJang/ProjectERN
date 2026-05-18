// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_WorldCameraShake.generated.h"

class UCameraShakeBase;

/**
 * 범위 공격용 카메라 흔들림 Notify
 * 노티 위치 기준으로 PlayWorldCameraShake 호출 → 반경 내 모든 플레이어 카메라가 거리 감쇠와 함께 흔들림
 * 노티가 모든 클라에서 fire되므로 별도 RPC 불필요
 */
UCLASS(DisplayName = "World Camera Shake")
class PROJECTERN_API UAnimNotify_WorldCameraShake : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override { return TEXT("World Camera Shake"); }

	// 재생할 카메라 흔들림 클래스
	UPROPERTY(EditAnywhere, Category = "CameraShake")
	TSubclassOf<UCameraShakeBase> ShakeClass;

	// 흔들림 강도 배율 (거리 감쇠와 별개로 곱해짐)
	UPROPERTY(EditAnywhere, Category = "CameraShake", meta = (ClampMin = "0.0"))
	float Scale = 1.f;

	// 이 반경 안은 풀 강도 (cm)
	UPROPERTY(EditAnywhere, Category = "CameraShake", meta = (ClampMin = "0.0"))
	float InnerRadius = 300.f;

	// 이 반경 밖은 0 (cm)
	UPROPERTY(EditAnywhere, Category = "CameraShake", meta = (ClampMin = "0.0"))
	float OuterRadius = 1500.f;

	// 감쇠 곡선 (1.0 = 선형, 2.0 = 제곱)
	UPROPERTY(EditAnywhere, Category = "CameraShake", meta = (ClampMin = "0.0"))
	float Falloff = 1.f;

	// 액터 위치에서의 오프셋 (머리 위 마법 임팩트 등)
	UPROPERTY(EditAnywhere, Category = "CameraShake")
	FVector OriginOffset = FVector::ZeroVector;

	// 흔들림이 임팩트 중심을 향하도록 회전 보정
	UPROPERTY(EditAnywhere, Category = "CameraShake")
	bool bOrientShakeTowardsEpicenter = false;
};
