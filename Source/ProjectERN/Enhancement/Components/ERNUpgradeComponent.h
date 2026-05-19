#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Enhancement/Data/ERNUpgradeTypes.h"
#include "ERNUpgradeComponent.generated.h"

class IERNUpgradeDataProvider;

/**
 * UERNUpgradeComponent - 강화 Consumer + 네트워크 브릿지
 * 
 * 상점의 UERNShopComponent와 동일한 위임 패턴(Proxy Pattern)을 사용합니다.
 * - 클라이언트: UI 입력을 받아 Server RPC로 서버에 강화 요청
 * - 서버: Provider에게 검증/처리를 위임하고 결과를 Client RPC로 응답
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTERN_API UERNUpgradeComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UERNUpgradeComponent();

    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    // ===== 공개 API =====

    /** 강화 UI 열기 (모루 액터에서 호출) */
    UFUNCTION(BlueprintCallable, Category = "Upgrade")
    void OpenUpgradeUI(AActor* TargetAnvil);

    /** 강화 UI 닫기 */
    UFUNCTION(BlueprintCallable, Category = "Upgrade")
    void CloseUpgradeUI();

    /** 아이템 선택 시 강화 미리보기 데이터 생성 */
    UFUNCTION(BlueprintCallable, Category = "Upgrade")
    FERNUpgradePreview GetUpgradePreview(int32 SlotIndex);

    /** 강화 시도 (E키 확인 시) */
    UFUNCTION(BlueprintCallable, Category = "Upgrade")
    void TryUpgradeItem(int32 SlotIndex);

    /** 강화 UI 열림 상태 */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Upgrade")
    bool IsUpgradeUIOpen() const { return bIsUpgradeOpen; }

    // ===== UI 갱신 이벤트 =====

    UPROPERTY(BlueprintAssignable, Category = "Upgrade")
    FOnUpgradeComplete OnUpgradeResult;

    UPROPERTY(BlueprintAssignable, Category = "Upgrade")
    FOnUpgradeUIUpdate OnUpgradeUIUpdate;

protected:
    // ===== Server RPC =====

    UFUNCTION(Server, Reliable, Category = "Upgrade")
    void Server_RequestUpgrade(int32 SlotIndex, AActor* TargetAnvil);

    // ===== Client RPC =====

    UFUNCTION(Client, Reliable, Category = "Upgrade")
    void Client_UpgradeResult(const FERNUpgradeTransaction& Transaction);

    // ===== 내부 콜백 =====

    UFUNCTION()
    void OnUpgradeComplete(const FERNUpgradeTransaction& Transaction);

private:
    // Provider 참조 (GameInstance에서 획득)
    IERNUpgradeDataProvider* DataProvider = nullptr;

    bool bIsUpgradeOpen = false;

    UPROPERTY()
    AActor* CurrentTargetAnvil = nullptr;

    // Provider 획득 헬퍼
    void AcquireProvider();
};
