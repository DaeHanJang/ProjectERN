// Copyright Epic Games, Inc. All Rights Reserved.

#include "Shop/Actors/ERNShopActor.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Blueprint/UserWidget.h"
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

    // 상호작용 범위 (스피어)
    InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionSphere"));
    InteractionSphere->SetupAttachment(RootComponent);
    InteractionSphere->SetSphereRadius(200.f);
    InteractionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    InteractionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
    InteractionSphere->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

    // 상호작용 프롬프트 위젯 컴포넌트
    InteractionPromptWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("InteractionPromptWidget"));
    InteractionPromptWidget->SetupAttachment(RootComponent);
    InteractionPromptWidget->SetWidgetSpace(EWidgetSpace::World);
    InteractionPromptWidget->SetVisibility(false); // 기본적으로 숨김
}

void AERNShopActor::BeginPlay()
{
    Super::BeginPlay();

    // 오버랩 이벤트 바인딩
    InteractionSphere->OnComponentBeginOverlap.AddDynamic(this, &AERNShopActor::OnSphereBeginOverlap);
    InteractionSphere->OnComponentEndOverlap.AddDynamic(this, &AERNShopActor::OnSphereEndOverlap);
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
    if (!ERNPC || !ERNPC->ShopMainWidgetClass)
    {
        UE_LOG(LogShopProvider, Warning, TEXT("[ShopActor] 유효하지 않은 PlayerController이거나 ShopMainWidgetClass가 없습니다."));
        return;
    }

    // 위젯 생성 및 뷰포트 추가
    if (UUserWidget* Widget = CreateWidget<UUserWidget>(PlayerController, ERNPC->ShopMainWidgetClass))
    {
        Widget->AddToViewport(100);

        // 마우스 커서 표시 및 입력 모드 변경
        PlayerController->SetShowMouseCursor(true);
        PlayerController->SetInputMode(FInputModeUIOnly());
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

void AERNShopActor::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent,
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
            
        if (InteractionPromptWidget)
        {
            InteractionPromptWidget->SetVisibility(true);
        }
    }
}

void AERNShopActor::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent,
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
            
        if (InteractionPromptWidget)
        {
            InteractionPromptWidget->SetVisibility(false);
        }
    }
}
