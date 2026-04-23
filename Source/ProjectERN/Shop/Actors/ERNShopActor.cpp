// Copyright Epic Games, Inc. All Rights Reserved.

#include "Shop/Actors/ERNShopActor.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Character/Player/ERNPlayerController.h"
#include "Character/Player/ProjectERNCharacter.h"
#include "Shop/Components/ERNShopComponent.h"
#include "Shop/Provider/ERNDummyShopProvider.h"

AERNShopActor::AERNShopActor()
{
    PrimaryActorTick.bCanEverTick = false;

    // 루트 컴포넌트
    MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
    RootComponent = MeshComponent;

    // 상호작용 박스
    InteractionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionBox"));
    InteractionBox->SetupAttachment(RootComponent);
    InteractionBox->SetBoxExtent(FVector(150.f, 150.f, 100.f));
    InteractionBox->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
    InteractionBox->SetGenerateOverlapEvents(true);

    // 오버랩 이벤트 바인딩
    InteractionBox->OnComponentBeginOverlap.AddDynamic(
        this, &AERNShopActor::OnInteractionBeginOverlap);
    InteractionBox->OnComponentEndOverlap.AddDynamic(
        this, &AERNShopActor::OnInteractionEndOverlap);
}

// ===== IInteractable 구현 =====

void AERNShopActor::Interact_Implementation(APlayerController* PlayerController)
{
    if (!bIsShopActive)
    {
        UE_LOG(LogShopProvider, Warning, TEXT("[ShopActor] 상점 비활성 상태: %s"), *ShopID.ToString());
        return;
    }

    AERNPlayerController* ERNPC = Cast<AERNPlayerController>(PlayerController);
    if (!ERNPC)
    {
        UE_LOG(LogShopProvider, Warning, TEXT("[ShopActor] 유효하지 않은 PlayerController"));
        return;
    }

    // 플레이어 캐릭터에서 ShopComponent 획득
    AProjectERNCharacter* Character = Cast<AProjectERNCharacter>(ERNPC->GetPawn());
    if (!Character)
    {
        UE_LOG(LogShopProvider, Warning, TEXT("[ShopActor] 플레이어 캐릭터를 찾을 수 없음"));
        return;
    }

    UERNShopComponent* ShopComp = Character->GetShopComponent();
    if (!ShopComp)
    {
        UE_LOG(LogShopProvider, Warning, TEXT("[ShopActor] 플레이어 캐릭터에서 ShopComponent를 찾을 수 없음"));
        return;
    }

    // 상점 열기 (자신을 TargetNPC로 전달하여 위임 패턴에 사용)
    UE_LOG(LogShopProvider, Log, TEXT("[ShopActor] 상점 열기 요청: %s (%s)"),
        *ShopDisplayName.ToString(), *ShopID.ToString());
    ShopComp->OpenShop(ShopID, this);
}

bool AERNShopActor::CanInteract_Implementation() const
{
    return bIsShopActive;
}

FText AERNShopActor::GetInteractionText_Implementation() const
{
    if (bIsShopActive)
    {
        return InteractionPrompt;
    }
    return FText::FromString(TEXT("이용할 수 없음"));
}

// ===== 오버랩 이벤트 =====

void AERNShopActor::OnInteractionBeginOverlap(UPrimitiveComponent* OverlappedComponent,
    AActor* OtherActor, UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    AProjectERNCharacter* Character = Cast<AProjectERNCharacter>(OtherActor);
    if (!Character) return;

    // PlayerController에 현재 상호작용 가능한 액터 설정
    AERNPlayerController* PC = Cast<AERNPlayerController>(Character->GetController());
    if (PC)
    {
        PC->SetCurrentInteractable(this);
        UE_LOG(LogShopProvider, Verbose, TEXT("[ShopActor] 상호작용 범위 진입: %s"),
            *Character->GetName());
    }
}

void AERNShopActor::OnInteractionEndOverlap(UPrimitiveComponent* OverlappedComponent,
    AActor* OtherActor, UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex)
{
    AProjectERNCharacter* Character = Cast<AProjectERNCharacter>(OtherActor);
    if (!Character) return;

    AERNPlayerController* PC = Cast<AERNPlayerController>(Character->GetController());
    if (PC)
    {
        PC->ClearCurrentInteractable();
        UE_LOG(LogShopProvider, Verbose, TEXT("[ShopActor] 상호작용 범위 이탈: %s"),
            *Character->GetName());
    }
}
