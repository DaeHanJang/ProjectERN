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
 * 
 * [리팩토링] 아이템 테이블(FERNItemTable)의 NextGradeItemID를 직접 참조하는 방식으로 전환.
 *            기존 FERNUpgradePathTable 별도 캐시 방식은 제거되었습니다.
 */
class IERNUpgradeDataProvider
{
    GENERATED_BODY()

public:
    /** Provider 초기화 */
    UFUNCTION(BlueprintNativeEvent, Category = "Upgrade")
    void Initialize(UObject* Owner);

    /** 특정 아이템의 강화 정보 조회 (아이템 테이블 NextGradeItemID 기반, 재료 수량은 1개 고정) */
    UFUNCTION(BlueprintNativeEvent, Category = "Upgrade")
    bool GetUpgradeInfo(FName SourceItemID, FName& OutResultItemID, FName& OutMaterialID);

    /** 강화 트랜잭션 처리 (서버 측에서 호출) */
    UFUNCTION(BlueprintNativeEvent, Category = "Upgrade")
    void ProcessUpgrade(FERNUpgradeTransaction Transaction, ACharacter* PlayerChar);

    /** 데이터 준비 완료 여부 */
    UFUNCTION(BlueprintNativeEvent, Category = "Upgrade")
    bool IsDataReady();
};
