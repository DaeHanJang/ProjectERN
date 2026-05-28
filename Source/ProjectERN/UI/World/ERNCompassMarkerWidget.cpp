// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/World/ERNCompassMarkerWidget.h"

#include "Components/Image.h"

void UERNCompassMarkerWidget::SetIcon(UTexture2D* IconTexture)
{
	if (IconImage && IconTexture)
	{
		IconImage->SetBrushFromTexture(IconTexture);
	}
}
