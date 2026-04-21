// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
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
	// UI 아이콘 텍스처
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSoftObjectPtr<UTexture2D> Icon = nullptr;
	
};
