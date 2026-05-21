#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/IInteractable.h"
#include "ERNUpgradeActor.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UWidgetComponent;

/**
 * AERNUpgradeActor - 강화 모루 액터
 * AERNShopActor와 동일한 IInteractable 패턴을 사용합니다.
 */
UCLASS()
class PROJECTERN_API AERNUpgradeActor : public AActor, public IInteractable
{
    GENERATED_BODY()

public:
    AERNUpgradeActor();

    virtual void BeginPlay() override;

    // ===== IInteractable 구현 =====
    virtual void Interact_Implementation(APlayerController* PlayerController) override;
    virtual bool CanInteract_Implementation() const override;
    virtual FText GetInteractionText_Implementation() const override;
    virtual void EndInteract_Implementation(APlayerController* PlayerController) override;
    virtual void ActivateInteract_Implementation() const override;
    virtual EInteractionExecutionPolicy GetInteractionExecutionPolicy_Implementation() const override;

protected:
    // 에디터에서 설정할 프로퍼티
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade")
    FText InteractionPrompt = FText::FromString(TEXT("무기 강화"));

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade")
    bool bIsActive = true;

    // 에디터에서 할당할 강화 메인 UI 위젯 클래스 (WBP_UpgradeMain)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Upgrade|UI")
    TSubclassOf<UUserWidget> UpgradeWidgetClass;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UStaticMeshComponent> MeshComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<USphereComponent> InteractionSphere;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UWidgetComponent> InteractionPromptWidget;

private:
    UPROPERTY()
    UUserWidget* ActiveUpgradeWidget = nullptr;

    UFUNCTION()
    void HandleUpgradeClosed();
};
