// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ERNItemAssetLoadTypes.h"
#include "Engine/DataAsset.h"
#include "ItemDataAssetBase.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTERN_API UItemDataAssetBase : public UDataAsset
{
	GENERATED_BODY()
	
public:
	// 로드 정책에 따른 Soft reference 로드 목록 반환
	virtual void GatherSoftPaths(const EItemAssetLoadFlags LoadFlags, TArray<FSoftObjectPath>& OutPaths) const;
	
public:
	// UI 아이콘 텍스처
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftObjectPtr<UTexture2D> Icon = nullptr;
	
	// 스태틱 메시
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftObjectPtr<UStaticMesh> StaticMesh = nullptr;
	
	// 스켈레탈 메시 (스태틱 메시가 없을 경우)
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftObjectPtr<USkeletalMesh> SkeletalMesh = nullptr;
	
	// 메시 트랜스폼
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FTransform Transform = FTransform::Identity;
	
	// 사운드
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftObjectPtr<USoundBase> Sound = nullptr;
	
};
