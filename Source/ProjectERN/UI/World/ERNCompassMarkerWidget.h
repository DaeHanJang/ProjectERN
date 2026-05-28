// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ERNCompassMarkerWidget.generated.h"

class UImage;
class UTexture2D;

/**
 * 나침반 바 위에 표시되는 아이콘 마커 (플레이어/핀/자기장 공용)
 */
UCLASS()
class PROJECTERN_API UERNCompassMarkerWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void SetIcon(UTexture2D* IconTexture);

private:
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> IconImage;
};
