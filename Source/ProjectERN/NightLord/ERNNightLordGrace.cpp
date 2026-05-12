// Fill out your copyright notice in the Description page of Project Settings.


#include "NightLord/ERNNightLordGrace.h"
#include "Character/ERNCharacterBase.h"
#include "Character/Player/ProjectERNCharacter.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Blueprint/UserWidget.h"
#include "Character/Player/ERNPlayerController.h"
#include "GAS/ERNAttributeSet.h"
#include "UI/ERNInteractableWidget.h"
#include "UI/ERNUIManagerSubsystem.h"

AERNNightLordGrace::AERNNightLordGrace()
{
	PrimaryActorTick.bCanEverTick = false;
	
	// GraceMesh 설정
	GraceMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GraceMesh"));
	RootComponent = GraceMesh;
	
	// InteractionComponent 설정
	InteractionComponent = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionComponent"));
	InteractionComponent->SetupAttachment(RootComponent);
	InteractionComponent->SetSphereRadius(300.f);
	InteractionComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	
	// InteractionOrinotWudget 초기화
	InteractionPromptWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("InteractionPromptWidget"));
	InteractionPromptWidget->SetupAttachment(RootComponent);
	InteractionPromptWidget->SetWidgetSpace(EWidgetSpace::World);
	InteractionPromptWidget->SetVisibility(false);
	
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
			InteractableWidget->OnWidgetClosed.AddDynamic(this, &AERNNightLordGrace::HandlePopupClosed);
			
		}
		FInputModeGameAndUI InputMode;
		InputMode.SetWidgetToFocus(LevelUpPopupWidget->TakeWidget()); // 포커스 지정!
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PlayerController->SetInputMode(InputMode);
		
		PlayerController->SetShowMouseCursor(true);
		//PlayerController->SetInputMode(FInputModeGameAndUI());
		
		//TODO 디버그 로그 (패키징 시 제거)
		UE_LOG(LogTemp, Warning, TEXT("화톳불 : 레벨업 팝업 알림"));
	}
}

void AERNNightLordGrace::EndInteract_Implementation(APlayerController* PlayerController)
{
	AERNPlayerController* ERNPC = Cast<AERNPlayerController>(PlayerController);
	if (!ERNPC) return;
	
	// 상호작용 대상 해제
	ERNPC->ClearCurrentInteractable();
	
	// 프롬프트 숨기기
	if (InteractionPromptWidget)
	{
		InteractionPromptWidget->SetVisibility(false);
	}
	
	if (LevelUpPopupWidget)
	{
		if (UERNInteractableWidget* InteractableWidget = Cast<UERNInteractableWidget>(LevelUpPopupWidget))
		{
			InteractableWidget->BP_PlayCloseAnimation();
		}
		else
		{
			LevelUpPopupWidget->RemoveFromViewport();
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
	
	
}

bool AERNNightLordGrace::CanInteract_Implementation() const
{
	return true;
}

FText AERNNightLordGrace::GetInteractionText_Implementation() const
{
	return FText::FromString(TEXT("E : 휴식"));
}

EInteractionExecutionPolicy AERNNightLordGrace::GetInteractionExecutionPolicy_Implementation() const
{
	return EInteractionExecutionPolicy::LocalOnly;
}

void AERNNightLordGrace::BeginPlay()
{
	Super::BeginPlay();
	
	InteractionComponent->OnComponentBeginOverlap.AddDynamic(this, &AERNNightLordGrace::OnSphereBeginOverlap);
	InteractionComponent->OnComponentEndOverlap.AddDynamic(this, &AERNNightLordGrace::OnSphereEndOverlap);
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
				LevelUpPopupWidget->RemoveFromViewport();
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
	// 플레이어 캐릭터를 불러옴
	AProjectERNCharacter* PlayerCharacter = Cast<AProjectERNCharacter>(OtherActor); 
	
	// 플레이어가 아니면 여기서 즉시 함수 종료 (안전장치)
	if (!PlayerCharacter) 
	{
		return;
	}

	// 회복 함수 호출
	RestoreAttributes(PlayerCharacter);
	
	// 상호작용 등록 및 UI 표시
	AERNPlayerController* PC = Cast<AERNPlayerController>(PlayerCharacter->GetController());
	
	if (PC)
	{
		PC->SetCurrentInteractable(this);
		
		if (InteractionPromptWidget)
		{
			InteractionPromptWidget->SetVisibility(true);
		}
	}
}

void AERNNightLordGrace::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	AProjectERNCharacter* PlayerCharacter = Cast<AProjectERNCharacter>(OtherActor);
	if (!PlayerCharacter) return;
	
	AERNPlayerController* PC = Cast<AERNPlayerController>(PlayerCharacter->GetController());
	if (PC)
	{
		EndInteract_Implementation(PC);
	}
}

void AERNNightLordGrace::RestoreAttributes(AProjectERNCharacter* TargetCharacter)
{
	// 캐릭터가 맞다면 캐릭터가 가지고 있는 체력, 마나 등이 들어 있는 어트리뷰트 셋을 가져옴  
	UERNAttributeSet* AttributeSet = TargetCharacter->GetAttributeSet();
	if (!AttributeSet) return; 
	// 어트리뷰트셋이 없다면 리턴
	
	// AttributeSet에 있는 체력,마나,스테미나를 게터로 불러온 Max변수의 값에 맞춰서 셋
	AttributeSet->SetHealth(AttributeSet->GetMaxHealth());
	AttributeSet->SetMana(AttributeSet->GetMaxMana());
	AttributeSet->SetStamina(AttributeSet->GetMaxStamina());
	
	//TODO 디버그로그
	UE_LOG(LogTemp, Warning, TEXT("%s: 화톳불에 의해 모든 스탯이 회복되었습니다.!"), *TargetCharacter->GetName());
}
