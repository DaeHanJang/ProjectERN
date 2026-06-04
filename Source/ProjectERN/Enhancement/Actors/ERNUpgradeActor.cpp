#include "Enhancement/Actors/ERNUpgradeActor.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Blueprint/UserWidget.h"
#include "Character/Player/ERNPlayerController.h"
#include "Character/Player/ProjectERNCharacter.h"
#include "Enhancement/Components/ERNUpgradeComponent.h"
#include "Enhancement/Data/ERNUpgradeTypes.h"
#include "UI/ERNInteractableWidget.h"
#include "UI/ERNUIManagerSubsystem.h"

AERNUpgradeActor::AERNUpgradeActor()
{
    PrimaryActorTick.bCanEverTick = false;

    MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
    RootComponent = MeshComponent;

    InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionSphere"));
    InteractionSphere->SetupAttachment(RootComponent);
    InteractionSphere->SetSphereRadius(200.f);
    InteractionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    InteractionSphere->SetCollisionResponseToAllChannels(ECR_Overlap);
    InteractionSphere->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);

    InteractionPromptWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("InteractionPromptWidget"));
    InteractionPromptWidget->SetupAttachment(RootComponent);
    InteractionPromptWidget->SetWidgetSpace(EWidgetSpace::World);
    InteractionPromptWidget->SetVisibility(false);
}

void AERNUpgradeActor::BeginPlay()
{
    Super::BeginPlay();
}

void AERNUpgradeActor::Interact_Implementation(APlayerController* PlayerController)
{
    if (!bIsActive) return;

    AERNPlayerController* ERNPC = Cast<AERNPlayerController>(PlayerController);
    if (!ERNPC) return;

    if (ActiveUpgradeWidget) return;

    // UI 매니저 게이트: 다른 UI(인벤토리, 상점 등)가 열려있으면 차단
    ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer();
    if (!LocalPlayer) return;

    UERNUIManagerSubsystem* UIManager = LocalPlayer->GetSubsystem<UERNUIManagerSubsystem>();
    if (!UIManager) return;

    if (!UIManager->RequestOpenUI(EERNUIType::Upgrade))
    {
        UE_LOG(LogUpgrade, Warning, TEXT("[UpgradeActor] RequestOpenUI(Upgrade) 거부됨! 다른 UI가 열려있음."));
        return;
    }

    UE_LOG(LogUpgrade, Warning, TEXT("[DEBUG] Interact_Implementation 시작"));

    if (!UpgradeWidgetClass)
    {
        UE_LOG(LogUpgrade, Error, TEXT("[DEBUG] UpgradeWidgetClass가 설정되지 않았습니다!"));
        return;
    }

    UE_LOG(LogUpgrade, Warning, TEXT("[DEBUG] CreateWidget 호출 직전 (Class: %s)"), *UpgradeWidgetClass->GetName());
    ActiveUpgradeWidget = CreateWidget<UUserWidget>(PlayerController, UpgradeWidgetClass);
    
    if (ActiveUpgradeWidget)
    {
        UE_LOG(LogUpgrade, Warning, TEXT("[DEBUG] 위젯 생성 성공. AddToViewport 호출"));
        ActiveUpgradeWidget->AddToViewport();
        ActiveUpgradeWidget->SetFocus();

        if (UERNInteractableWidget* InteractableWidget = Cast<UERNInteractableWidget>(ActiveUpgradeWidget))
        {
            InteractableWidget->OnWidgetClosed.AddDynamic(this, &AERNUpgradeActor::HandleUpgradeClosed);
            UE_LOG(LogUpgrade, Warning, TEXT("[DEBUG] OnWidgetClosed 델리게이트 바인딩 성공"));
        }
    }
    else
    {
        UE_LOG(LogUpgrade, Error, TEXT("[DEBUG] CreateWidget 실패!"));
    }

    // 플레이어 캐릭터에서 UpgradeComponent 획득
    AProjectERNCharacter* Character = Cast<AProjectERNCharacter>(ERNPC->GetPawn());
    if (!Character) return;

    UERNUpgradeComponent* UpgradeComp = Character->GetUpgradeComponent();
    if (!UpgradeComp) return;

    UpgradeComp->OpenUpgradeUI(this);

    // 마우스 커서 표시 + 입력 모드 변경
    PlayerController->SetShowMouseCursor(true);
    FInputModeGameAndUI InputMode;
    InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
    PlayerController->SetInputMode(InputMode);

    UE_LOG(LogUpgrade, Log, TEXT("[UpgradeActor] 강화 UI 열기"));
}

bool AERNUpgradeActor::CanInteract_Implementation() const
{
    return bIsActive;
}

FText AERNUpgradeActor::GetInteractionText_Implementation() const
{
    return bIsActive ? InteractionPrompt : FText::FromString(TEXT("이용할 수 없음"));
}

void AERNUpgradeActor::EndInteract_Implementation(APlayerController* PlayerController)
{
    if (InteractionPromptWidget) InteractionPromptWidget->SetVisibility(false);

    if (ActiveUpgradeWidget)
    {
        if (UERNInteractableWidget* IW = Cast<UERNInteractableWidget>(ActiveUpgradeWidget))
        {
            IW->BP_PlayCloseAnimation();
        }
        else
        {
            ActiveUpgradeWidget->RemoveFromParent();
            ActiveUpgradeWidget = nullptr;

            AERNPlayerController* ERNPC = Cast<AERNPlayerController>(PlayerController);
            if (ERNPC)
            {
                ERNPC->SetInputMode(FInputModeGameOnly());
                ERNPC->SetShowMouseCursor(false);
                
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
}

void AERNUpgradeActor::HandleUpgradeClosed()
{
    if (ActiveUpgradeWidget)
    {
        ActiveUpgradeWidget->RemoveFromParent();
        ActiveUpgradeWidget = nullptr;
    }

    if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
    {
        PC->SetShowMouseCursor(false);
        FInputModeGameOnly InputMode;
        PC->SetInputMode(InputMode);

        if (AProjectERNCharacter* Character = Cast<AProjectERNCharacter>(PC->GetPawn()))
        {
            if (UERNUpgradeComponent* UpgradeComp = Character->GetUpgradeComponent())
            {
                UpgradeComp->CloseUpgradeUI();
            }
        }

        AERNPlayerController* ERNPC = Cast<AERNPlayerController>(PC);
        if (ERNPC)
        {
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

void AERNUpgradeActor::ActivateInteract_Implementation() const
{
    if (InteractionPromptWidget) InteractionPromptWidget->SetVisibility(true);
}

EInteractionExecutionPolicy AERNUpgradeActor::GetInteractionExecutionPolicy_Implementation() const
{
    return EInteractionExecutionPolicy::LocalOnly;
}
