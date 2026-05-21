#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Inventory/Item/Data/ERNItemEnums.h"
#include "ERNUpgradePreviewWidget.generated.h"

class UTextBlock;
class UImage;

/**
 * 강화 전/후 스탯 비교 프리뷰 위젯
 * 하나의 클래스를 2개 인스턴스로 사용 (Before / After)
 */
UCLASS()
class PROJECTERN_API UERNUpgradePreviewWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    /**
     * 프리뷰 데이터 세팅
     * @param DisplayName  아이템 이름
     * @param Grade        아이템 등급
     * @param LightAtk     약공격 대미지
     * @param HeavyAtk     강공격 대미지
     * @param bIsAfter     true면 "강화 후" 모드 (증가분 초록색 표시)
     */
    UFUNCTION(BlueprintCallable, Category = "Upgrade UI")
    void SetPreviewData(const FText& DisplayName, EItemGrade Grade,
                        int32 LightAtk, int32 HeavyAtk, bool bIsAfter,
                        UTexture2D* WeaponIcon = nullptr);

    /** 프리뷰 초기화 (아이템 미선택 상태) */
    UFUNCTION(BlueprintCallable, Category = "Upgrade UI")
    void ClearPreview();

protected:
    // === 바인딩 위젯 ===

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> ItemNameText;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> GradeText;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> LightAttackText;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> HeavyAttackText;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UImage> GradeBorderImage;  // 등급 색상 테두리

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UImage> WeaponIconImage;  // 무기 아이콘

private:
    /** 등급에 따른 색상 반환 */
    FLinearColor GetGradeColor(EItemGrade Grade) const;

    /** 등급에 따른 텍스트 반환 */
    FText GetGradeDisplayText(EItemGrade Grade) const;
};
