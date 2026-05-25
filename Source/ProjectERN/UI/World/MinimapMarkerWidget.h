// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MinimapMarkerWidget.generated.h"

class AERNMinimapTargetPoint;
class UTexture2D;
class UImage;
/**
 * 
 */
UCLASS()
class PROJECTERN_API UMinimapMarkerWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	void SetMarkerData(UTexture2D* IconTexture, AERNMinimapTargetPoint* Target);
	
private:
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UImage> IconImage;
	
	TWeakObjectPtr<AERNMinimapTargetPoint> SourceTarget;
	
};
