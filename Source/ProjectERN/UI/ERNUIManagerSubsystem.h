// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "ERNUIManagerSubsystem.generated.h"

/**
 * UI 타입 열거형 - 전체 화면 UI의 종류를 정의
 */
UENUM(BlueprintType)
enum class EERNUIType : uint8
{
	None		UMETA(Hidden),
	Shop		UMETA(DisplayName = "Shop"),
	Inventory	UMETA(DisplayName = "Inventory"),
	LevelUp		UMETA(DisplayName = "LevelUp"),
	Upgrade		UMETA(DisplayName = "Upgrade"),
	Minimap		UMETA(DisplayName = "Minimap"),
	PauseMenu	UMETA(DisplayName = "PauseMenu"),
	DHRoll      UMETA(DisplayName = "DHRoll")
};

/**
 * UI 상태 변경 델리게이트
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUIStateChanged, EERNUIType, UIType);

/**
 * UI 매니저 서브시스템
 * 
 * 여러 전체 화면 UI(상점, 인벤토리, 레벨업 등)가 동시에 열리지 않도록 중앙 관리합니다.
 * ULocalPlayerSubsystem 기반으로, 로컬 플레이어당 하나만 존재하며
 * 어디서든 GetSubsystem<UERNUIManagerSubsystem>()으로 접근 가능합니다.
 */
UCLASS()
class PROJECTERN_API UERNUIManagerSubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:
	// ── 핵심 API ──
	
	UFUNCTION(BlueprintCallable, Category = "UI")
	void ResetUIState();

	/** UI 열기 요청. 다른 UI가 이미 열려있으면 false 반환 */
	UFUNCTION(BlueprintCallable, Category = "UI")
	bool RequestOpenUI(EERNUIType UIType);

	/** 현재 활성 UI 닫기 */
	UFUNCTION(BlueprintCallable, Category = "UI")
	void CloseActiveUI();

	/** 현재 열려있는 UI 타입 반환 */
	UFUNCTION(BlueprintPure, Category = "UI")
	EERNUIType GetActiveUIType() const { return ActiveUIType; }

	/** 구매 확인 팝업 등록 (기존 팝업이 있다면 닫음) */
	UFUNCTION(BlueprintCallable, Category = "UI|Popup")
	void RegisterConfirmPurchasePopup(class UERNConfirmPurchaseWidget* NewPopup);

	/** 현재 열려있는 구매 확인 팝업 닫기 */
	UFUNCTION(BlueprintCallable, Category = "UI|Popup")
	void CloseConfirmPurchasePopup();

	/** UI 상태 변경 시 호출되는 이벤트 */
	UPROPERTY(BlueprintAssignable, Category = "UI|Events")
	FOnUIStateChanged OnUIStateChanged;

private:
	/** 현재 활성화된 UI 타입 */
	EERNUIType ActiveUIType = EERNUIType::None;

	/** 현재 열려있는 구매 확인 팝업 참조 */
	UPROPERTY()
	TObjectPtr<class UERNConfirmPurchaseWidget> ActiveConfirmPurchasePopup = nullptr;
};
