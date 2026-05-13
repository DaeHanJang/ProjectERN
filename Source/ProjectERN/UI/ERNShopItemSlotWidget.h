#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Shop/Data/ERNShopTypes.h"
#include "ERNShopItemSlotWidget.generated.h"

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

protected:
	// 마우스가 슬롯에 들어왔을 때 오버라이드
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	
	// 마우스가 슬롯에서 나갔을 때 오버라이드
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;
};
