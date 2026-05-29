// Fill out your copyright notice in the Description page of Project Settings.

#include "NightLord/ERNNightLordGrace.h"

#include "NiagaraComponent.h"
#include "Character/ERNCharacterBase.h"
#include "Character/Player/ProjectERNCharacter.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Blueprint/UserWidget.h"
#include "Character/Player/ERNPlayerController.h"
#include "Components/AudioComponent.h"
#include "GAS/ERNAttributeSet.h"
#include "UI/ERNInteractableWidget.h"
#include "UI/ERNUIManagerSubsystem.h"

AERNNightLordGrace::AERNNightLordGrace()
{
	PrimaryActorTick.bCanEverTick = false;
	SetReplicates(true);
	
	// Collision
	InteractionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionComponent"));
	SetRootComponent(InteractionComponent);
	InteractionComponent->InitSphereRadius(250.0f);
	InteractionComponent->SetCollisionProfileName(TEXT("OverlapAll"));
	InteractionComponent->SetGenerateOverlapEvents(true);
	
	// Mesh
	GraceMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GraceMesh"));
	GraceMesh->SetupAttachment(GetRootComponent());
	GraceMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
	// Effect
	EffectComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("EffectComponent"));
	EffectComponent->SetupAttachment(GetRootComponent());
	
	// Sound
	SoundComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("SoundComponent"));
	SoundComponent->SetupAttachment(GetRootComponent());
	
	// Prompt
	InteractionPromptWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("InteractionPromptWidget"));
	InteractionPromptWidget->SetupAttachment(GetRootComponent());
	InteractionPromptWidget->SetWidgetSpace(EWidgetSpace::World);
	InteractionPromptWidget->SetVisibility(false);
}

void AERNNightLordGrace::BeginPlay()
{
	Super::BeginPlay();
	
	// Collision Binding
	InteractionComponent->OnComponentBeginOverlap.AddUniqueDynamic(this, &AERNNightLordGrace::OnSphereBeginOverlap);
}

void AERNNightLordGrace::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (InteractionComponent)
	{
		InteractionComponent->OnComponentBeginOverlap.RemoveDynamic(this, &AERNNightLordGrace::OnSphereBeginOverlap);
	}

	Super::EndPlay(EndPlayReason);
}

void AERNNightLordGrace::Interact_Implementation(APlayerController* PlayerController)
{
	if (!PlayerController) return;
	
	AERNPlayerController* ERNPC = Cast<AERNPlayerController>(PlayerController);
	if (!ERNPC) return;

	if (!ERNPC->LevelUpWidgetClass)
	{
		UE_LOG(LogTemp, Error, TEXT("화톳불 오류: 플레이어 컨트롤러에 LevelUpWidgetClass가 할당되지 않았습니다. (BP_PlayerController 확인)"));
		return;
	}
	
	if (LevelUpPopupWidget)
	{
		UE_LOG(LogTemp, Warning, TEXT("화톳불: 팝업 위젯이 이미 존재합니다."));
		return;
	}
	
	// UI 매니저 게이트: 다른 UI(상점, 인벤토리)가 열려있으면 차단
	if (ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer())
	{
		if (UERNUIManagerSubsystem* UIManager = LocalPlayer->GetSubsystem<UERNUIManagerSubsystem>())
		{
			if (!UIManager->RequestOpenUI(EERNUIType::LevelUp))
			{
				UE_LOG(LogTemp, Warning, TEXT("화톳불 : 다른 UI가 열려있어 레벨업 UI를 열 수 없습니다."));
				return;
			}
		}
	}
	
	LevelUpPopupWidget = CreateWidget<UUserWidget>(PlayerController, ERNPC->LevelUpWidgetClass);
	
	if (LevelUpPopupWidget)
	{
		LevelUpPopupWidget->AddToViewport(100);
		
		// 베이스 위젯인지 캐스팅하여 델리게이트 바인딩
		if (UERNInteractableWidget* InteractableWidget = Cast<UERNInteractableWidget>(LevelUpPopupWidget))
		{
			// 혹시 이미 바인드 되어 있을지 모르므로 제거 후 연결 - 예외 처리
			InteractableWidget->OnWidgetClosed.RemoveDynamic(this, &AERNNightLordGrace::HandlePopupClosed);
			InteractableWidget->OnWidgetClosed.AddUniqueDynamic(this, &AERNNightLordGrace::HandlePopupClosed);
		}
		
		FInputModeGameAndUI InputMode;
		InputMode.SetWidgetToFocus(LevelUpPopupWidget->TakeWidget()); // 포커스 지정!
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		InputMode.SetHideCursorDuringCapture(true);
		PlayerController->SetInputMode(InputMode);
		PlayerController->SetShowMouseCursor(true);
	}
}

bool AERNNightLordGrace::CanInteract_Implementation() const
{
	return true;
}

void AERNNightLordGrace::ActivateInteract_Implementation() const
{
	if (InteractionPromptWidget)
	{
		InteractionPromptWidget->SetVisibility(true);
	}
}

void AERNNightLordGrace::EndInteract_Implementation(APlayerController* PlayerController)
{
	AERNPlayerController* ERNPC = Cast<AERNPlayerController>(PlayerController);
	if (!ERNPC)
	{
		return;
	}
	
	if (LevelUpPopupWidget)
	{
		if (UERNInteractableWidget* InteractableWidget = Cast<UERNInteractableWidget>(LevelUpPopupWidget))
		{
			InteractableWidget->BP_PlayCloseAnimation();
		}
		else
		{
			LevelUpPopupWidget->RemoveFromParent();
			LevelUpPopupWidget = nullptr; 
			
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
	
	if (InteractionPromptWidget)
	{
		InteractionPromptWidget->SetVisibility(false);
	}
}

FText AERNNightLordGrace::GetInteractionText_Implementation() const
{
	return FText::FromString(TEXT("레벨 업"));
}

EInteractionExecutionPolicy AERNNightLordGrace::GetInteractionExecutionPolicy_Implementation() const
{
	return EInteractionExecutionPolicy::LocalOnly;
}

void AERNNightLordGrace::HandlePopupClosed()
{
	if (AProjectERNCharacter* PlayerChar = Cast<AProjectERNCharacter>(GetWorld()->GetFirstPlayerController()->GetCharacter()))
	{
		AERNPlayerController* ERNPC = Cast<AERNPlayerController>(PlayerChar->GetController());
		if (ERNPC)
		{
			if (LevelUpPopupWidget)
			{
				LevelUpPopupWidget->RemoveFromParent();
				LevelUpPopupWidget = nullptr; 
				
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
				
				UE_LOG(LogTemp, Warning, TEXT("화톳불 : 상호작용 종료 및 UI 애니메이션 완료 닫힘"));
			}
		}
	}
}

void AERNNightLordGrace::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                              UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// 서버 검증
	if (!HasAuthority())
	{
		return;
	}
	
	// 플레이어 캐릭터 검증 후 스테이터스 회복 
	if (const AProjectERNCharacter* PlayerCharacter = Cast<AProjectERNCharacter>(OtherActor)) 
	{
		if (OtherComp == PlayerCharacter->GetRootComponent())
		{
			RestoreAttributes(PlayerCharacter);
		}
	}
}

void AERNNightLordGrace::RestoreAttributes(const AProjectERNCharacter* TargetCharacter) const
{
	if (!TargetCharacter)
	{
		return;
	}
	
	UERNAttributeSet* AttributeSet = TargetCharacter->GetAttributeSet();
	if (!AttributeSet)
	{
		return;
	}
	
	// 체력, 마나, 스테미나, 성배병 최대치 회복
	AttributeSet->SetHealth(AttributeSet->GetMaxHealth());
	AttributeSet->SetMana(AttributeSet->GetMaxMana());
	AttributeSet->SetStamina(AttributeSet->GetMaxStamina());
	AttributeSet->SetFlaskQuantity(AttributeSet->GetMaxFlaskQuantity());
	
	UE_LOG(LogTemp, Warning, TEXT("%s HP: %f, MP: %f, Stamina: %f, Flask: %f"), *GetNameSafe(TargetCharacter), AttributeSet->GetHealth(), AttributeSet->GetMana(), AttributeSet->GetStamina(), AttributeSet->GetFlaskQuantity());
}
