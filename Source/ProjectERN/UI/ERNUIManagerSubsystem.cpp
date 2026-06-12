// Fill out your copyright notice in the Description page of Project Settings.

#include "UI/ERNUIManagerSubsystem.h"
#include "UI/ERNConfirmPurchaseWidget.h"
#include "Subsystem/ERNCutsceneSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogUIManager, Log, All);

void UERNUIManagerSubsystem::ResetUIState()
{
	ActiveUIType = EERNUIType::None;
	ActiveConfirmPurchasePopup = nullptr;
}

bool UERNUIManagerSubsystem::RequestOpenUI(EERNUIType UIType)
{
	if (UIType == EERNUIType::None)
	{
		return false;
	}
	
	// 이미 같은 UI가 열려있으면 → 이미 열린 상태이므로 성공 반환 (중복 호출 안전)
	if (ActiveUIType == UIType)
	{
		return true;
	}

	// 컷신 중에는 전체화면 UI(메뉴, 인벤토리 등)를 열 수 없도록 차단
	if (UGameInstance* GI = GetLocalPlayer()->GetGameInstance())
	{
		if (UERNCutsceneSubsystem* Cutscene = GI->GetSubsystem<UERNCutsceneSubsystem>())
		{
			if (Cutscene->IsPlayingCutscene())
			{
				UE_LOG(LogUIManager, Warning, TEXT("[UIManager] 컷신 재생 중이므로 UI [%d] 열기가 차단되었습니다."), (int32)UIType);
				return false;
			}
		}
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
	
	OnUIStateChanged.Broadcast(ActiveUIType);

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
	
	// 전체 UI가 닫힐 때, 연관된 서브 팝업들도 전부 닫기
	CloseConfirmPurchasePopup();

	OnUIStateChanged.Broadcast(ActiveUIType);

	UE_LOG(LogUIManager, Log, TEXT("[UIManager] UI [%d] 닫힘"), (int32)ClosedType);
}

void UERNUIManagerSubsystem::RegisterConfirmPurchasePopup(UERNConfirmPurchaseWidget* NewPopup)
{
	// 이미 열려있는 팝업이 있다면 취소(닫기) 처리
	if (ActiveConfirmPurchasePopup && ActiveConfirmPurchasePopup != NewPopup)
	{
		ActiveConfirmPurchasePopup->OnNoClicked(); // 버튼 상태를 복구하고 팝업을 닫음
	}
	
	ActiveConfirmPurchasePopup = NewPopup;
}

void UERNUIManagerSubsystem::CloseConfirmPurchasePopup()
{
	if (ActiveConfirmPurchasePopup)
	{
		ActiveConfirmPurchasePopup->OnNoClicked();
		ActiveConfirmPurchasePopup = nullptr;
	}
}
