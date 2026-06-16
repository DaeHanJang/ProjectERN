#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ERNItemToolTipWidget.generated.h"

class UTextBlock;
class UImage;
class UPanelWidget;

/**
 * 상점 아이템 툴팁 위젯 (C++ 기반)
 */
UCLASS()
class PROJECTERN_API UERNItemToolTipWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// 툴팁 데이터 갱신 (런타임 상태 포함)
	UFUNCTION(BlueprintCallable, Category = "Shop|ToolTip")
	void UpdateTooltipWithState(const struct FItemRuntimeState& ItemState, int32 ItemPrice);

	// 툴팁 데이터 갱신 (순수 데이터테이블 기반, 상점용)
	UFUNCTION(BlueprintCallable, Category = "Shop|ToolTip")
	void UpdateTooltip(FName ItemID, int32 ItemPrice);

	// 상태 텍스트 설정 (장착 중, 선택됨 등)
	UFUNCTION(BlueprintCallable, Category = "Shop|ToolTip")
	void SetTooltipStateText(const FText& InStateText);

protected:
	virtual void NativeConstruct() override;

	// 헤더 상태 텍스트 (옵션)
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* Text_TooltipState;

	// 필수로 연결해야 하는 위젯들
	UPROPERTY(meta = (BindWidget))
	UTextBlock* ItemNameText;

	UPROPERTY(meta = (BindWidget))
	UImage* TooltipIcon;

	// 등급에 따른 배경 틴트 이미지 (선택적 연결)
	UPROPERTY(meta = (BindWidgetOptional))
	UImage* GradeBackgroundImage;

	// 나중에 추가로 엮을 스탯 패널 및 텍스트 (아직 블루프린트에 없다면 에러가 나지 않도록 Optional 처리)
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* PriceText;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* DescriptionText;

	UPROPERTY(meta = (BindWidgetOptional))
	UPanelWidget* StatsPanel;

	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* DamageText;

	UPROPERTY(meta = (BindWidgetOptional))
	UPanelWidget* AbilityPanel;

	UPROPERTY(meta = (BindWidgetOptional))
	UImage* AbilityIcon;

	// 새로 추가: 아이템의 무작위 어빌리티 텍스트 출력용
	UPROPERTY(meta = (BindWidgetOptional))
	UTextBlock* AbilityPlusText;
};
