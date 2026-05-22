#pragma once

#include "CoreMinimal.h"
#include "Inventory/Item/Data/ERNItemEnums.h"
#include "ERNUpgradeTypes.generated.h"

// ===== 로그 카테고리 =====
DECLARE_LOG_CATEGORY_EXTERN(LogUpgrade, Log, All);

// ===== 열거형 =====

UENUM(BlueprintType)
enum class EUpgradeResult : uint8
{
    Success,
    MaterialInsufficient,   // 단석 부족
    AlreadyMaxGrade,        // 이미 최고 등급
    InvalidItem,            // 유효하지 않은 아이템
    NoUpgradePath,          // 강화 경로 없음 (NextGradeItemID가 None)
    InventoryError,         // 인벤토리 교체 실패
};

/**
 * 강화 미리보기 데이터 - UI에서 강화 전/후 비교에 사용
 */
USTRUCT(BlueprintType)
struct FERNUpgradePreview
{
    GENERATED_BODY()

    // 강화 전 아이템 정보
    UPROPERTY(BlueprintReadOnly, Category = "Upgrade")
    FName SourceItemID;

    UPROPERTY(BlueprintReadOnly, Category = "Upgrade")
    FText SourceDisplayName;

    UPROPERTY(BlueprintReadOnly, Category = "Upgrade")
    EItemGrade SourceGrade = EItemGrade::None;

    UPROPERTY(BlueprintReadOnly, Category = "Upgrade")
    int32 SourceLightAttack = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Upgrade")
    int32 SourceHeavyAttack = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Upgrade")
    TSoftObjectPtr<UTexture2D> SourceIcon;

    // 강화 후 아이템 정보
    UPROPERTY(BlueprintReadOnly, Category = "Upgrade")
    FName ResultItemID;

    UPROPERTY(BlueprintReadOnly, Category = "Upgrade")
    FText ResultDisplayName;

    UPROPERTY(BlueprintReadOnly, Category = "Upgrade")
    EItemGrade ResultGrade = EItemGrade::None;

    UPROPERTY(BlueprintReadOnly, Category = "Upgrade")
    int32 ResultLightAttack = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Upgrade")
    int32 ResultHeavyAttack = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Upgrade")
    TSoftObjectPtr<UTexture2D> ResultIcon;

    // 필요 재료 정보 (소모 수량은 1개 고정)
    UPROPERTY(BlueprintReadOnly, Category = "Upgrade")
    FName RequiredMaterialID;

    UPROPERTY(BlueprintReadOnly, Category = "Upgrade")
    FText MaterialDisplayName;
    
    UPROPERTY(BlueprintReadOnly, Category = "Upgrade")
    int32 CurrentMaterialCount = 0;  // UI 표기용 현재 보유 수량

    UPROPERTY(BlueprintReadOnly, Category = "Upgrade")
    TSoftObjectPtr<UTexture2D> MaterialIcon;

    UPROPERTY(BlueprintReadOnly, Category = "Upgrade")
    EItemGrade MaterialGrade = EItemGrade::None;

    // 강화 가능 여부 (재료 1개 이상 보유 시 true)
    UPROPERTY(BlueprintReadOnly, Category = "Upgrade")
    bool bCanUpgrade = false;
};

/**
 * 강화 트랜잭션 - 강화 요청 및 결과를 담는 구조체
 */
USTRUCT(BlueprintType)
struct FERNUpgradeTransaction
{
    GENERATED_BODY()

    // 강화 대상 인벤토리 슬롯 인덱스
    UPROPERTY(BlueprintReadWrite, Category = "Upgrade")
    int32 SlotIndex = INDEX_NONE;

    // 강화 전 아이템
    UPROPERTY(BlueprintReadWrite, Category = "Upgrade")
    FName SourceItemID;

    // 강화 후 아이템
    UPROPERTY(BlueprintReadWrite, Category = "Upgrade")
    FName ResultItemID;

    // 소모 재료 아이템 (1개 고정 소모)
    UPROPERTY(BlueprintReadWrite, Category = "Upgrade")
    FName MaterialItemID;

    // 결과
    UPROPERTY(BlueprintReadWrite, Category = "Upgrade")
    EUpgradeResult Result = EUpgradeResult::Success;

    // 요청한 플레이어 (멀티플레이 식별용)
    UPROPERTY(BlueprintReadWrite, Category = "Upgrade")
    AActor* RequestingPlayer = nullptr;

    // 타임스탬프
    UPROPERTY(BlueprintReadWrite, Category = "Upgrade")
    float Timestamp = 0.f;
};

// ===== 델리게이트 =====

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUpgradeComplete, const FERNUpgradeTransaction&, Transaction);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnUpgradeUIUpdate);
