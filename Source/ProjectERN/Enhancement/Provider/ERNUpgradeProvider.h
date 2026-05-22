#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Enhancement/Provider/ERNUpgradeDataProvider.h"
#include "ERNUpgradeProvider.generated.h"

class UERNInventoryComponent;
class UItemManagerSubsystem;

/**
 * 아이템 테이블(FERNItemTable) 기반 강화 Provider
 * 
 * [리팩토링] 기존 DT_UpgradePath 별도 캐시 방식을 제거하고,
 * ItemManagerSubsystem::FindItemRow()를 통해 아이템 테이블의 NextGradeItemID를 직접 조회합니다.
 */
UCLASS(Blueprintable)
class PROJECTERN_API UERNUpgradeProvider : public UObject, public IERNUpgradeDataProvider
{
    GENERATED_BODY()

public:
    UERNUpgradeProvider();

    // === IERNUpgradeDataProvider 구현 ===
    virtual void Initialize_Implementation(UObject* Owner) override;
    virtual bool GetUpgradeInfo_Implementation(FName SourceItemID, FName& OutResultItemID, FName& OutMaterialID) override;
    virtual void ProcessUpgrade_Implementation(FERNUpgradeTransaction Transaction, ACharacter* PlayerChar) override;
    virtual bool IsDataReady_Implementation() override;

    // === 델리게이트 ===
    UPROPERTY(BlueprintAssignable)
    FOnUpgradeComplete OnUpgradeComplete;

private:
    UPROPERTY()
    UObject* OwnerObject = nullptr;

    /** ItemManagerSubsystem 캐시 참조 (Initialize 시 획득) */
    UItemManagerSubsystem* CachedItemMgr = nullptr;
};
