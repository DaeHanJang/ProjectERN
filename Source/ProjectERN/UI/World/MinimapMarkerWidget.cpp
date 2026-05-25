// Fill out your copyright notice in the Description page of Project Settings.


#include "MinimapMarkerWidget.h"
#include "InputCoreTypes.h"
#include "Components/Image.h"
#include "World/ERNMinimapTargetPoint.h"

void UMinimapMarkerWidget::SetMarkerData(UTexture2D* IconTexture, AERNMinimapTargetPoint* Target)
{
	SourceTarget = Target;
	
	if (IconImage && IconTexture)
	{
		IconImage->SetBrushFromTexture(IconTexture);
	}
}
