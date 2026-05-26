// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/IInteractable.h"
#include "Inventory/Item/Data/ERNItemEnums.h"
#include "Shop/Data/ERNShopTypes.h"
#include "ERNShopActor.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UWidgetComponent;

/**
 * AERNShopActor - 월드에 배치하는 상점 NPC/오브젝트
 * 
 * IInteractable 인터페이스를 구현하여 기존 상호작용 시스템과 통합합니다.
 * 플레이어가 E키로 상호작용하면 해당 캐릭터의 ShopComponent::OpenShop()을 호출합니다.
 */
UCLASS(BlueprintType, Blueprintable)
class PROJECTERN_API AERNShopActor : public AActor, public IInteractable
{
    GENERATED_BODY()

public:
    AERNShopActor();

    // ===== IInteractable 구현 =====

    /** 상호작용 실행 - 상점 열기 */
    virtual void Interact_Implementation(APlayerController* PlayerController) override;

    /** 상호작용 가능 여부 */
    virtual bool CanInteract_Implementation() const override;

    /** 상호작용 UI 텍스트 */
    virtual FText GetInteractionText_Implementation() const override;

    /** 상호작용 활성화 (프롬프트 켜기) */
    virtual void ActivateInteract_Implementation() const override;

    /** 상호작용 종료 */
    virtual void EndInteract_Implementation(APlayerController* PlayerController) override;

    /** 상호작용 실행 정책 */
    virtual EInteractionExecutionPolicy GetInteractionExecutionPolicy_Implementation() const override;

    
protected:
    virtual void BeginPlay() override;
    
    // ==== UI에서 호출될 델리게이트 수신 함수 ====
    UFUNCTION()
    void HandleShopClosed();
    
    UPROPERTY()
    class UUserWidget* ActiveShopWidget = nullptr;

    
    // ===== 컴포넌트 =====

    /** 상점 외형 메시 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* MeshComponent;

    /** 상호작용 감지 범위 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    USphereComponent* InteractionSphere;

    /** 상호작용 프롬프트 위젯 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UWidgetComponent* InteractionPromptWidget;

    // ===== 상점 설정 =====

    /** 상인 고유 ID (예: "ForestMerchant_01") - 캐시 식별 및 무작위 저장용 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop")
    FName ShopID;
    
    /** 진열할 아이템의 카테고리별 슬롯 갯수 구성표 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop")
    TArray<FERNShopSlotConfig> SlotConfigs;

    /** 이 상점의 타입 (World, Boss 등) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop")
    EShopType ShopType = EShopType::World;

    /** 상점 이름 (UI 표시용) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop")
    FText ShopDisplayName = FText::FromString(TEXT("상점"));

    /** 상호작용 안내 텍스트 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop")
    FText InteractionPrompt = FText::FromString(TEXT("상점 열기"));

    /** 상점 활성화 여부 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop")
    bool bIsShopActive = true;


};
