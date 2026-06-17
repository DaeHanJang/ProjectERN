// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_Skill_SpawnProjectile.generated.h"

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

	// 스폰 기준점에서 로컬 방향으로 추가 이동 (X=전방, Y=우측, Z=상단). 노티파이마다 개별 설정
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Projectile")
	FVector SpawnLocationOffset = FVector::ZeroVector;
};
