// Fill out your copyright notice in the Description page of Project Settings.


#include "MinimapMarkerWidget.h"
#include "InputCoreTypes.h"
#include "Character/Player/ERNPlayerController.h"
#include "Components/Image.h"
#include "World/ERNMinimapPinPoint.h"
#include "World/ERNMinimapTargetPoint.h"

void UMinimapMarkerWidget::SetMarkerData(UTexture2D* IconTexture, AERNMinimapTargetPoint* Target)
{
	SourceTarget = Target;
	
	if (IconImage && IconTexture)
	{
		IconImage->SetBrushFromTexture(IconTexture);
	}
}

FReply UMinimapMarkerWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// 마우스 휠 키 (핀) 입력에 대한 처리
	if (InMouseEvent.GetEffectingButton() != EKeys::MiddleMouseButton)
	{
		return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
	}
	
	AERNMinimapPinPoint* Pin = Cast<AERNMinimapPinPoint>(SourceTarget.Get());
	if (Pin == nullptr)
	{
		return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
	}
	
	AERNPlayerController* PC = Cast<AERNPlayerController>(GetOwningPlayer());
	if (PC == nullptr)
	{
		return FReply::Handled();
	}
	
	// 본인 핀을 눌렀을 때 삭제
	if (Pin->GetOwnerPlayerState() == PC->PlayerState)
	{
		PC->Server_RequestRemoveMinimapPin(Pin);
	}
	
	return FReply::Handled();
}
