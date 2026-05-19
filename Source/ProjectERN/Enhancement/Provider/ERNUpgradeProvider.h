#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Enhancement/Provider/ERNUpgradeDataProvider.h"
#include "ERNUpgradeProvider.generated.h"

class UERNInventoryComponent;

/**
 * DataTable 기반 강화 Provider
 * DT_UpgradePath 테이블을 캐싱하여 강화 경로 조회 및 트랜잭션 처리를 담당합니다.
 */
UCLASS(Blueprintable)
class PROJECTERN_API UERNUpgradeProvider : public UObject, public IERNUpgradeDataProvider
{
    GENERATED_BODY()

public:
    UERNUpgradeProvider();

    // === IERNUpgradeDataProvider 구현 ===
    virtual void Initialize_Implementation(UObject* Owner) override;
    virtual bool GetUpgradePath_Implementation(FName SourceItemID, FERNUpgradePathTable& OutPath) override;
    virtual void ProcessUpgrade_Implementation(FERNUpgradeTransaction Transaction, ACharacter* PlayerChar) override;
    virtual bool IsDataReady_Implementation() override;

    // === 델리게이트 ===
    UPROPERTY(BlueprintAssignable)
    FOnUpgradeComplete OnUpgradeComplete;

protected:
    // 블루프린트에서 할당할 강화 경로 데이터 테이블
    UPROPERTY(EditDefaultsOnly, Category = "Upgrade Data")
    TObjectPtr<UDataTable> UpgradePathTable;

private:
    // SourceItemID → 강화 경로 캐시 맵
    TMap<FName, FERNUpgradePathTable> CachedPaths;

    UPROPERTY()
    UObject* OwnerObject = nullptr;

    bool bIsDataCached = false;
};
