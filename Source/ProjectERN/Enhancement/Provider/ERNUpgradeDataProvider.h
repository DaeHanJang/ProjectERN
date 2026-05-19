#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Enhancement/Data/ERNUpgradeTypes.h"
#include "ERNUpgradeDataProvider.generated.h"

UINTERFACE(MinimalAPI, BlueprintType)
class UERNUpgradeDataProvider : public UInterface
{
    GENERATED_BODY()
};

/**
 * 강화 Provider 인터페이스
 * 강화 경로 조회, 검증, 처리를 추상화합니다.
 */
class IERNUpgradeDataProvider
{
    GENERATED_BODY()

public:
    /** Provider 초기화 (경로 테이블 캐싱) */
    UFUNCTION(BlueprintNativeEvent, Category = "Upgrade")
    void Initialize(UObject* Owner);

    /** 특정 아이템의 강화 경로 조회 (캐시에서) */
    UFUNCTION(BlueprintNativeEvent, Category = "Upgrade")
    bool GetUpgradePath(FName SourceItemID, FERNUpgradePathTable& OutPath);

    /** 강화 트랜잭션 처리 (서버 측에서 호출) */
    UFUNCTION(BlueprintNativeEvent, Category = "Upgrade")
    void ProcessUpgrade(FERNUpgradeTransaction Transaction, ACharacter* PlayerChar);

    /** 데이터 준비 완료 여부 */
    UFUNCTION(BlueprintNativeEvent, Category = "Upgrade")
    bool IsDataReady();
};
