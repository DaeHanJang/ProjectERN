#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Shop/Data/ERNShopTypes.h"
#include "ERNShopItemSlotWidget.generated.h"

class UERNConfirmPurchaseWidget;
class UTextBlock;

// 슬롯 호버 관련 델리게이트 선언
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSlotHoveredSignature, const FERNShopItemData&, SlotData);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSlotUnhoveredSignature);

/**
 * 상점 아이템 슬롯 위젯의 C++ 부모 클래스
 */
UCLASS()
class PROJECTERN_API UERNShopItemSlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 슬롯이 보유하고 있는 아이템 데이터 (블루프린트에서 Spawn 시 할당받을 수 있도록 설정)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop|Slot", meta = (ExposeOnSpawn = "true"))
	FERNShopItemData ItemData;

	// 마우스 호버 시작 시 브로드캐스트
	UPROPERTY(BlueprintAssignable, Category = "Shop|Slot")
	FOnSlotHoveredSignature OnSlotHovered;

	// 마우스 호버 종료 시 브로드캐스트
	UPROPERTY(BlueprintAssignable, Category = "Shop|Slot")
	FOnSlotUnhoveredSignature OnSlotUnhovered;

	// 구매 버튼 클릭 시 호출 (블루프린트에서 바인딩)
	UFUNCTION(BlueprintCallable, Category = "Shop|Slot")
	void OnPurchaseButtonClicked();

	// 구매 확인 팝업 위젯 클래스 (블루프린트에서 할당)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Shop|Slot")
	TSubclassOf<UERNConfirmPurchaseWidget> ConfirmPurchaseWidgetClass;

	// 버튼 활성/비활성 제어 (서버 응답 대기 중 중복 클릭 방지)
	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = "Shop|Slot")
	void SetPurchaseButtonEnabled(bool bEnabled);

	// 슬롯의 텍스트 및 UI를 갱신하는 함수
	UFUNCTION(BlueprintCallable, Category = "Shop|Slot")
	virtual void RefreshSlotUI();

protected:
	virtual void NativeConstruct() override;

	// 가격 표시 텍스트 블록 (블루프린트 위젯 이름을 PriceText로 맞춰야 연동됨)
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* PriceText;

	// 재고 표시 텍스트 블록 (블루프린트 위젯 이름을 StockText로 맞춰야 연동됨)
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* StockText;

	// 마우스가 슬롯에 들어왔을 때 오버라이드
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	
	// 마우스가 슬롯에서 나갔을 때 오버라이드
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;
};
