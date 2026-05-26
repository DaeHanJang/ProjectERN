// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MinimapPlayerMarkerWidget.generated.h"

class UImage;
class APawn;
class UTexture2D;

/**
 * 
 */
UCLASS()
class PROJECTERN_API UMinimapPlayerMarkerWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	void SetPlayerPawn(APawn* InPawn);
	void UpdateMarker(const FVector2D& MapPosition, float YawDegrees);
	void SetIconTexture(UTexture2D* IconTexture);
	
private:
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UImage> IconImage;
		
	TWeakObjectPtr<APawn> SourcePawn;
};
