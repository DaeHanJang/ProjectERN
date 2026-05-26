// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MinimapNightRainZoneWidget.generated.h"

class UImage;
class UTexture2D;
/**
 * 
 */
UCLASS()
class PROJECTERN_API UMinimapNightRainZoneWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	void UpdateZone(const FVector2D& MapCenter, const FVector2D& MapRadius);
	
protected:
	virtual void NativePreConstruct() override;
	
private:
	void ApplyZoneTexture();
	
private:
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UImage> ZoneImage;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Minimap|NightRain", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UTexture2D> ZoneTexture;
};
