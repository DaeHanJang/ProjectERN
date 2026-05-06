// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/ERNUIManagerSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogUIManager, Log, All);

bool UERNUIManagerSubsystem::RequestOpenUI(EERNUIType UIType)
{
	if (UIType == EERNUIType::None)
	{
		return false;
	}
	
	// 이미 같은 UI가 열려있으면 → 토글 닫기
	if (ActiveUIType == UIType)
	{
		CloseActiveUI();
		return false;
	}

	// 다른 UI가 열려있으면 → 거부
	if (ActiveUIType != EERNUIType::None)
	{
		UE_LOG(LogUIManager, Warning, TEXT("[UIManager] UI [%d] 가 이미 열려있어 UI [%d] 를 열 수 없습니다."),
			(int32)ActiveUIType, (int32)UIType);
		return false;
	}

	// 열기 허용
	ActiveUIType = UIType;
	OnUIOpened.Broadcast(UIType);
	
	UE_LOG(LogUIManager, Log, TEXT("[UIManager] UI [%d] 열림"), (int32)UIType);
	return true;
}

void UERNUIManagerSubsystem::CloseActiveUI()
{
	if (ActiveUIType == EERNUIType::None)
	{
		return;
	}

	EERNUIType ClosedType = ActiveUIType;
	ActiveUIType = EERNUIType::None;
	
	OnUIClosed.Broadcast(ClosedType);
	
	UE_LOG(LogUIManager, Log, TEXT("[UIManager] UI [%d] 닫힘"), (int32)ClosedType);
}
