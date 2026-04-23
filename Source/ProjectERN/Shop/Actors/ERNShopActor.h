// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/IInteractable.h"
#include "ERNShopActor.generated.h"

class UBoxComponent;
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

protected:

    // ===== 컴포넌트 =====

    /** 상점 외형 메시 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* MeshComponent;

    /** 상호작용 감지 범위 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UBoxComponent* InteractionBox;

    // ===== 상점 설정 =====

    /** 이 상점의 고유 ID (Provider에서 데이터를 가져올 때 사용) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop")
    FName ShopID = FName("Shop_Default");

    /** 상점 이름 (UI 표시용) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop")
    FText ShopDisplayName = FText::FromString(TEXT("상점"));

    /** 상호작용 안내 텍스트 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop")
    FText InteractionPrompt = FText::FromString(TEXT("상점 열기"));

    /** 상점 활성화 여부 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop")
    bool bIsShopActive = true;

    // ===== 오버랩 이벤트 =====

    UFUNCTION()
    void OnInteractionBeginOverlap(UPrimitiveComponent* OverlappedComponent,
        AActor* OtherActor, UPrimitiveComponent* OtherComp,
        int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

    UFUNCTION()
    void OnInteractionEndOverlap(UPrimitiveComponent* OverlappedComponent,
        AActor* OtherActor, UPrimitiveComponent* OtherComp,
        int32 OtherBodyIndex);
};
