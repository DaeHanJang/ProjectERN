// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Shop/Data/ERNShopTypes.h"
#include "ERNConfirmPurchaseWidget.generated.h"

class UERNShopComponent;
class UERNShopItemSlotWidget;
class UTextBlock;

/**
 * 상점 아이템 구매 전 Yes/No를 확인하는 팝업 위젯
 */
UCLASS()
class PROJECTERN_API UERNConfirmPurchaseWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 팝업 초기화 – 아이템 데이터를 받아 UI에 표시
	UFUNCTION(BlueprintCallable, Category = "Shop|Purchase")
	void SetupPopup(const FERNShopItemData& InItemData);

	// 구매 확정 시 호출할 ShopComponent 참조
	UPROPERTY(BlueprintReadWrite, Category = "Shop|Purchase", meta = (ExposeOnSpawn = "true"))
	UERNShopComponent* ShopComponentRef = nullptr;

	// 팝업을 호출한 슬롯 위젯 (취소 시 버튼 복구용)
	UPROPERTY(BlueprintReadWrite, Category = "Shop|Purchase")
	UERNShopItemSlotWidget* OwnerSlotWidget = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> ItemNameText;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> PriceText;

	// Yes 버튼 클릭 핸들러
	UFUNCTION(BlueprintCallable, Category = "Shop|Purchase")
	void OnYesClicked();

	// No 버튼 클릭 핸들러 (UI 매니저에서도 호출 가능하도록 public으로 공개)
	UFUNCTION(BlueprintCallable, Category = "Shop|Purchase")
	void OnNoClicked();

	// 콘솔 키 처리
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	
protected:
	virtual void NativeConstruct() override;

	// 바탕(블러/배경) 클릭 시 팝업을 닫도록 입력 처리 오버라이드
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	// 아이템 데이터 (팝업에 표시할 정보)
	UPROPERTY(BlueprintReadOnly, Category = "Shop|Purchase")
	FERNShopItemData CachedItemData;
	
	// 블루프린트에서 UI 요소를 업데이트할 수 있도록 이벤트를 제공
	UFUNCTION(BlueprintImplementableEvent, Category = "Shop|Purchase")
	void BP_OnSetupPopup();
};
