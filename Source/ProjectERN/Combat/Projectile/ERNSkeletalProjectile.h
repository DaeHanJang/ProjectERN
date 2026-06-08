// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Combat/Projectile/ERNProjectileBase.h"
#include "ERNSkeletalProjectile.generated.h"

class USkeletalMeshComponent;

/**
 * AERNSkeletalProjectile - 스켈레탈 메시를 비주얼로 사용하는 투사체
 * 보스가 점프하며 사라질 때 보스 스켈레톤을 위로 빠르게 발사해
 * "위로 점프해 사라진 것처럼" 보이게 하는 연출용.
 * (이동/유도/속도/폭발 등은 베이스에서 그대로 처리)
 */
UCLASS()
class PROJECTERN_API AERNSkeletalProjectile : public AERNProjectileBase
{
	GENERATED_BODY()

public:
	AERNSkeletalProjectile();

public:
	// 투사체 비주얼 스켈레탈 메시 (보스 스켈레톤 등)
	// BP 컴포넌트 디테일에서 Skeletal Mesh Asset / Anim Class 지정
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile")
	TObjectPtr<USkeletalMeshComponent> SkeletalMesh;
};
