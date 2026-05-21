#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Inventory/Item/Data/ERNItemEnums.h"
#include "ERNUpgradeMaterialTooltipWidget.generated.h"

class UTextBlock;
class UImage;
class UTexture2D;

/**
 * 강화 재료(단석) 정보 툴팁 위젯
 * 필요한 단석의 아이콘, 이름, 보유/요구 개수, 등급별 테두리 색상을 표시합니다.
 */
UCLASS()
class PROJECTERN_API UERNUpgradeMaterialTooltipWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    /**
     * 재료 정보를 세팅합니다.
     * @param InMaterialName     단석 이름
     * @param InMaterialGrade    단석 등급
     * @param InCurrentCount     현재 보유 수량
     * @param InRequiredCount    필요 수량
     * @param InMaterialIcon     단석 아이콘 텍스처
     */
    UFUNCTION(BlueprintCallable, Category = "Upgrade UI")
    void SetMaterialData(const FText& InMaterialName, EItemGrade InMaterialGrade,
                         int32 InCurrentCount, int32 InRequiredCount,
                         UTexture2D* InMaterialIcon = nullptr);

    /** 표시 초기화 (아이템 미선택 상태) */
    UFUNCTION(BlueprintCallable, Category = "Upgrade UI")
    void ClearMaterial();

protected:
    // === 바인딩 위젯 ===

    /** 단석 이름 텍스트 */
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> MaterialNameText;

    /** 보유/요구 수량 텍스트 (예: "1 / 3") */
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> MaterialCountText;

    /** 단석 등급 텍스트 */
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> MaterialGradeText;

    /** 단석 아이콘 이미지 */
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UImage> MaterialIconImage;

    /** 등급별 테두리/필터 이미지 */
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UImage> GradeFilterImage;

private:
    /** 등급에 따른 색상 반환 */
    FLinearColor GetGradeColor(EItemGrade Grade) const;

    /** 등급에 따른 텍스트 반환 */
    FText GetGradeDisplayText(EItemGrade Grade) const;
};
