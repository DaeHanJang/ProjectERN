#pragma once

#include "CoreMinimal.h"
#include "UI/ERNInteractableWidget.h"
#include "Enhancement/Data/ERNUpgradeTypes.h"
#include "ERNUpgradeMainWidget.generated.h"

class UERNUpgradeComponent;
class UERNInventoryComponent;
class UERNUpgradeSlotWidget;
class UERNUpgradePreviewWidget;
class UERNUpgradeMaterialTooltipWidget;
class UScrollBox;
class UTextBlock;
class UButton;

/**
 * 강화 메인 UI 위젯
 * 레이아웃: [인벤토리 슬롯 목록] | [강화 전 툴팁] | [강화 후 툴팁] | [재료/확인 바]
 */
UCLASS()
class PROJECTERN_API UERNUpgradeMainWidget : public UERNInteractableWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

protected:
    // === 블루프린트 바인딩 위젯 ===

    // 인벤토리 장비 슬롯을 담을 ScrollBox
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UScrollBox> InventoryScrollBox;

    // 재료 정보 표시 (레거시 - MaterialTooltipWidget 사용 시 생략 가능)
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> MaterialNameText;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> MaterialCountText;

    // 슬롯 위젯 클래스 (블루프린트에서 할당)
    UPROPERTY(EditDefaultsOnly, Category = "Upgrade UI")
    TSubclassOf<UERNUpgradeSlotWidget> SlotWidgetClass;

    // 강화 전/후 프리뷰 위젯 (Phase 7에서 구현)
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UERNUpgradePreviewWidget> BeforePreviewWidget;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UERNUpgradePreviewWidget> AfterPreviewWidget;

    // 재료(단석) 정보 툴팁 위젯
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UERNUpgradeMaterialTooltipWidget> MaterialTooltipWidget;

    // 프리뷰 영역 전체를 감싸는 컨테이너 (화살표 이미지 등을 한 번에 숨기기 위함)
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UWidget> PreviewContainer;

    // 화살표 아이콘 개별 숨김용
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UWidget> PreviewArrow;

    // "강화 전", "강화 후" 상단 텍스트 숨김용
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UWidget> BeforeTitleText;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UWidget> AfterTitleText;

    // 강화 실행 버튼
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UButton> UpgradeButton;

    // === 내부 함수 ===

    /** 인벤토리에서 장비 아이템만 필터링하여 슬롯 생성 */
    void PopulateInventorySlots();

    /** 선택 초기화 (프리뷰 숨김 처리 등) */
    void ClearSelection();

    /** 슬롯 클릭 시 호출 (강화 미리보기 갱신) */
    UFUNCTION()
    void OnSlotSelected(int32 SlotIndex);

    /** 강화 확인 버튼 (E키) 클릭 시 */
    UFUNCTION(BlueprintCallable, Category = "Upgrade UI")
    void OnUpgradeConfirmed();

    /** 강화 결과 콜백 */
    UFUNCTION()
    void OnUpgradeResultReceived(const FERNUpgradeTransaction& Transaction);

    /** 인벤토리 변경(강화 후 데이터 동기화 완료 시점) 콜백 */
    UFUNCTION()
    void OnInventorySlotChanged(const FInventoryItemEntry& Entry);

    // 성공 이벤트를 블루프린트에서 연출하기 위함
    UFUNCTION(BlueprintImplementableEvent, Category = "Upgrade UI")
    void BP_OnUpgradeSuccess();

private:
    UPROPERTY()
    UERNUpgradeComponent* UpgradeCompRef = nullptr;

    UPROPERTY()
    UERNInventoryComponent* InventoryRef = nullptr;

    int32 SelectedSlotIndex = INDEX_NONE;
};
