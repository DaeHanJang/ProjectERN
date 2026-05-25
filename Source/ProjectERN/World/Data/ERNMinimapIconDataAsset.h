// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "World/Data/ERNMinimapIconTypes.h"
#include "ERNMinimapIconDataAsset.generated.h"

class UTexture2D;

/**
 * 
 */

UCLASS()
class PROJECTERN_API UERNMinimapIconDataAsset : public UDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Minimap")
	TMap<EERNMinimapIconType, TObjectPtr<UTexture2D>> IconTextures;
	
	UTexture2D* FindIconTexture(const EERNMinimapIconType Type) const;
};
