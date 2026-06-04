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
#include "UI/ERNInteractableWidget.h"
#include "UI/ERNUIManagerSubsystem.h"

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
    InteractionSphere->SetCollisionResponseToAllChannels(ECR_Overlap);
    InteractionSphere->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);

    // 상호작용 프롬프트 위젯 컴포넌트
    InteractionPromptWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("InteractionPromptWidget"));
    InteractionPromptWidget->SetupAttachment(RootComponent);
    InteractionPromptWidget->SetWidgetSpace(EWidgetSpace::World);
    InteractionPromptWidget->SetVisibility(false); // 기본적으로 숨김
}

void AERNShopActor::BeginPlay()
{
    Super::BeginPlay();

    // 하이브리드 ID 방식: 개발자가 명시적으로 ShopID를 지정하지 않았다면, 액터의 고유 인스턴스 이름을 사용
    if (ShopID.IsNone())
    {
        ShopID = GetFName();
    }
}

// ===== IInteractable 구현 =====

void AERNShopActor::Interact_Implementation(APlayerController* PlayerController)
{
    if (!bIsShopActive)
    {
        UE_LOG(LogShopProvider, Warning, TEXT("[ShopActor] 상점 비활성 상태: %d"), (int32)ShopType);
        return;
    }

    AERNPlayerController* ERNPC = Cast<AERNPlayerController>(PlayerController);
    if (!ERNPC || !ERNPC->ShopMainWidgetClass)
    {
        UE_LOG(LogShopProvider, Warning, TEXT("[ShopActor] 유효하지 않은 PlayerController이거나 ShopMainWidgetClass가 없습니다."));
        return;
    }
    
    if (ActiveShopWidget) return;
    
    // UI 매니저 게이트: 다른 UI(인벤토리, 레벨업)가 열려있으면 차단
    ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer();
    UE_LOG(LogShopProvider, Warning, TEXT("[ShopActor] ★ UI게이트 진입 - LocalPlayer: %s"), 
        LocalPlayer ? TEXT("유효") : TEXT("NULL"));
    
    if (!LocalPlayer)
    {
        UE_LOG(LogShopProvider, Warning, TEXT("[ShopActor] ★ LocalPlayer가 NULL! 상점 열기 중단."));
        return;
    }
    
    UERNUIManagerSubsystem* UIManager = LocalPlayer->GetSubsystem<UERNUIManagerSubsystem>();
    UE_LOG(LogShopProvider, Warning, TEXT("[ShopActor] ★ UIManager: %s"), 
        UIManager ? TEXT("유효") : TEXT("NULL"));
    
    if (!UIManager)
    {
        UE_LOG(LogShopProvider, Warning, TEXT("[ShopActor] ★ UIManager가 NULL! 상점 열기 중단."));
        return;
    }
    
    UE_LOG(LogShopProvider, Warning, TEXT("[ShopActor] ★ 현재 ActiveUI: %d, Shop 요청 시도"), 
        (int32)UIManager->GetActiveUIType());
    
    if (!UIManager->RequestOpenUI(EERNUIType::Shop))
    {
        UE_LOG(LogShopProvider, Warning, TEXT("[ShopActor] ★ RequestOpenUI(Shop) 거부됨! 다른 UI가 열려있음."));
        return;
    }
    
    UE_LOG(LogShopProvider, Warning, TEXT("[ShopActor] ★ RequestOpenUI(Shop) 성공! 상점 위젯 생성 진행."));
    
    ActiveShopWidget = CreateWidget<UUserWidget>(PlayerController, ERNPC->ShopMainWidgetClass);

    if (ActiveShopWidget)
    {
        ActiveShopWidget->AddToViewport(100);

        if (UERNInteractableWidget* InteractableWidget = Cast<UERNInteractableWidget>(ActiveShopWidget))
        {
            InteractableWidget->OnWidgetClosed.RemoveDynamic(this, &AERNShopActor::HandleShopClosed);
            InteractableWidget->OnWidgetClosed.AddDynamic(this, &AERNShopActor::HandleShopClosed);
        }
        
        PlayerController->SetShowMouseCursor(true);
        FInputModeGameAndUI InputMode;
        InputMode.SetWidgetToFocus(ActiveShopWidget->TakeWidget());
        InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        PlayerController->SetInputMode(InputMode);
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
    UE_LOG(LogShopProvider, Log, TEXT("[ShopActor] 상점 열기 요청: %s"), *ShopDisplayName.ToString());
    
    if (bIsFixedShop && FixedShopDataTable != nullptr)
    {
        ShopComp->OpenShopFixed(ShopID, ShopType, SlotConfigs, FixedShopDataTable, this);
    }
    else
    {
        ShopComp->OpenShopRandom(ShopID, ShopType, SlotConfigs, this);
    }
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

void AERNShopActor::EndInteract_Implementation(APlayerController* PlayerController)
{
    AERNPlayerController* ERNPC = Cast<AERNPlayerController>(PlayerController);
    if (!ERNPC) return;
    
    //상호작용 프롬프트 및 대상 해제
    if (InteractionPromptWidget)
    {
        InteractionPromptWidget->SetVisibility(false);
    }
    
    // 즉시 파괴하지 않고, 닫기 애니메이션을 재생하라고 명령
    if (ActiveShopWidget)
    {
        if (UERNInteractableWidget* InteractableWidget = Cast<UERNInteractableWidget>(ActiveShopWidget))
        {
            InteractableWidget->BP_PlayCloseAnimation();
        }
        else
        {
            // 혹시 베이스 클래스가 아닐 경우 예외 처리
            ActiveShopWidget->RemoveFromParent();
            ActiveShopWidget = nullptr;
            
            ERNPC->SetInputMode(FInputModeGameOnly());
            ERNPC->SetShowMouseCursor(false);
            
            // UI 매니저에 닫힘 알림
            if (ULocalPlayer* LocalPlayer = ERNPC->GetLocalPlayer())
            {
                if (UERNUIManagerSubsystem* UIManager = LocalPlayer->GetSubsystem<UERNUIManagerSubsystem>())
                {
                    UIManager->CloseActiveUI();
                }
            }
        }
    }
}

void AERNShopActor::HandleShopClosed()
{
    if (AProjectERNCharacter* PlayerChar = Cast<AProjectERNCharacter>(GetWorld()->GetFirstPlayerController()->GetCharacter()))
    {
        AERNPlayerController* ERNPC = Cast<AERNPlayerController>(PlayerChar->GetController());
        if (ERNPC)
        {
            // 여기서 위젯을 최종 파괴하고, 마우스 시점을 복구합니다.
            if (ActiveShopWidget)
            {
                ActiveShopWidget->RemoveFromParent();
                ActiveShopWidget = nullptr; 
                
                ERNPC->SetInputMode(FInputModeGameOnly());
                ERNPC->SetShowMouseCursor(false);
                
                // UI 매니저에 닫힘 알림
                if (ULocalPlayer* LocalPlayer = ERNPC->GetLocalPlayer())
                {
                    if (UERNUIManagerSubsystem* UIManager = LocalPlayer->GetSubsystem<UERNUIManagerSubsystem>())
                    {
                        UIManager->CloseActiveUI();
                    }
                }
                
                UE_LOG(LogShopProvider, Log, TEXT("[ShopActor] 상점 UI 애니메이션 종료 및 최종 닫힘"));
            }
        }
    }
}

void AERNShopActor::ActivateInteract_Implementation() const
{
    if (InteractionPromptWidget)
    {
        InteractionPromptWidget->SetVisibility(true);
    }
}

EInteractionExecutionPolicy AERNShopActor::GetInteractionExecutionPolicy_Implementation() const
{
    return EInteractionExecutionPolicy::LocalOnly;
}
