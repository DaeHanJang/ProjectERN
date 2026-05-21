#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ERNUpgradeSlotWidget.generated.h"

struct FERNItemTable;
class UTextBlock;
class UImage;
class UButton;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUpgradeSlotClicked, int32, SlotIndex);

UCLASS()
class PROJECTERN_API UERNUpgradeSlotWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;

    /** 슬롯 초기화 */
    void SetupSlot(int32 InSlotIndex, FName InItemID, const FERNItemTable* ItemRow);

    UPROPERTY(BlueprintAssignable)
    FOnUpgradeSlotClicked OnSlotClicked;

protected:
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UTextBlock> ItemNameText;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UImage> ItemIcon;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UImage> GradeBorder;  // 등급 색상 테두리

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UButton> SlotButton;

    UFUNCTION()
    void HandleButtonClicked();

private:
    int32 CachedSlotIndex = INDEX_NONE;
    FName CachedItemID;
};
