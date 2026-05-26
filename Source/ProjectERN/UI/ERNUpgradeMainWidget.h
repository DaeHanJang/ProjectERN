#pragma once

#include "CoreMinimal.h"
#include "UI/ERNInteractableWidget.h"
#include "Enhancement/Data/ERNUpgradeTypes.h"
#include "UI/ERNInventorySlotWidget.h"
#include "Inventory/Components/ERNEquipmentComponent.h"
#include "ERNUpgradeMainWidget.generated.h"

class UERNUpgradeComponent;
class UERNInventoryComponent;
class UERNUpgradePreviewWidget;
class UERNUpgradeMaterialTooltipWidget;
class UUniformGridPanel;
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

    // 인벤토리 장비 슬롯을 담을 격자 패널 (대안 3)
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UUniformGridPanel> UpgradeInventoryGrid;

    // 인벤토리 슬롯 위젯 클래스
    UPROPERTY(EditDefaultsOnly, Category = "Upgrade UI")
    TSubclassOf<UERNInventorySlotWidget> SlotWidgetClass;

    // 슬롯 기본 이미지
    UPROPERTY(EditDefaultsOnly, Category = "Upgrade UI")
    TSoftObjectPtr<UTexture2D> InventorySlotImage;

    // 슬롯 포커스 (클릭) 이미지
    UPROPERTY(EditDefaultsOnly, Category = "Upgrade UI")
    TSoftObjectPtr<UTexture2D> FocusSlotImage;

    // 장착 무기 슬롯
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UERNInventorySlotWidget> EquipableSlotWidget;

    // 재료 정보 표시 (레거시 - MaterialTooltipWidget 사용 시 생략 가능)
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> MaterialNameText;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> MaterialCountText;

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

    /** 인벤토리 슬롯 생성 (대안 3) */
    void PopulateInventorySlots();

    /** 시각적 상태 통합 갱신 및 프리뷰 갱신 */
    void UpdateVisuals();

    /** 선택 초기화 (프리뷰 숨김 처리 등) */
    void ClearSelection();

    /** 강화 확인 버튼 (E키) 클릭 시 */
    UFUNCTION(BlueprintCallable, Category = "Upgrade UI")
    void OnUpgradeConfirmed();

    /** 강화 결과 콜백 */
    UFUNCTION()
    void OnUpgradeResultReceived(const FERNUpgradeTransaction& Transaction);

    /** 인벤토리 변경(강화 후 데이터 동기화 완료 시점) 콜백 */
    UFUNCTION()
    void OnInventorySlotChanged(const FInventoryItemEntry& Entry);

    /** 장착 무기 변경 콜백 */
    UFUNCTION()
    void OnEquipableSlotChanged(const FInventoryItemEntry& Entry);

    // 성공 이벤트를 블루프린트에서 연출하기 위함
    UFUNCTION(BlueprintImplementableEvent, Category = "Upgrade UI")
    void BP_OnUpgradeSuccess();

    // 슬롯 인터랙션 콜백
    UFUNCTION()
    void OnSlotHoveredCallback(const int32 Index);

    UFUNCTION()
    void OnSlotUnhoveredCallback(const int32 Index);
    
    UFUNCTION()
    void OnSlotDoubleClickedCallback(const int32 Index);

    UFUNCTION()
    void OnSlotClickedCallback(const int32 Index);

    // 입력 처리 (키보드, 게임패드 포커스)
    virtual FReply NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
    virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

    // 타이머 처리용
    void UnhoverDelay();

    // 등급별 색상 반환
    FColor ItemGradeByColor(EItemGrade Grade);

private:
    UPROPERTY()
    UERNUpgradeComponent* UpgradeCompRef;

    UPROPERTY()
    UERNInventoryComponent* InventoryRef;

    // 생성된 슬롯 위젯들
    UPROPERTY()
    TArray<UERNInventorySlotWidget*> SlotWidgets;

    // 상태 관리 (인벤토리 로직 100% 동일화)
    int32 FocusSlotIndex = -1;
    int32 HoveredSlotIndex = -1;
    
    FTimerHandle UnhoverTimerHandle;
    
    const int32 ColumnSize = 4; // 인벤토리 가로 슬롯 개수
    
    const int32 GetNavigationTargetSlotIndex(const FKey& Key, const int32 MaxSlotSize, const int32 CurrentIndex) const;
};
