// Copyright Epic Games, Inc. All Rights Reserved.

#include "Actors/CharacterSelectionPedestal.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
#include "Character/Player/ERNPlayerController.h"

ACharacterSelectionPedestal::ACharacterSelectionPedestal()
{
	PrimaryActorTick.bCanEverTick = false;

	// 받침대 메시
	PedestalMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PedestalMesh"));
	RootComponent = PedestalMesh;

	// 상호작용 범위
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

void ACharacterSelectionPedestal::BeginPlay()
{
	Super::BeginPlay();

	InteractionSphere->OnComponentBeginOverlap.AddDynamic(this, &ACharacterSelectionPedestal::OnSphereBeginOverlap);
	InteractionSphere->OnComponentEndOverlap.AddDynamic(this, &ACharacterSelectionPedestal::OnSphereEndOverlap);
}

void ACharacterSelectionPedestal::Interact_Implementation(APlayerController* PlayerController)
{
	if (!PlayerController)
	{
		return;
	}

	// PlayerController의 위젯 클래스 사용
	AERNPlayerController* ERNPlayerController = Cast<AERNPlayerController>(PlayerController);
	if (!ERNPlayerController || !ERNPlayerController->CharacterSelectionWidgetClass)
	{
		return;
	}

	// 위젯 생성 및 표시
	if (UUserWidget* Widget = CreateWidget<UUserWidget>(PlayerController, ERNPlayerController->CharacterSelectionWidgetClass))
	{
		Widget->AddToViewport(100); // 높은 Z-Order

		// 마우스 커서 표시 및 입력 모드 변경
		UE_LOG(LogTemp, Warning, TEXT("위젯 생성"));
		PlayerController->SetShowMouseCursor(true);
		PlayerController->SetInputMode(FInputModeUIOnly());
	}
}

bool ACharacterSelectionPedestal::CanInteract_Implementation() const
{
	return true;
}

FText ACharacterSelectionPedestal::GetInteractionText_Implementation() const
{
	return FText::FromString(TEXT("E키를 눌러 캐릭터 선택"));
}

EInteractionExecutionPolicy ACharacterSelectionPedestal::GetInteractionExecutionPolicy_Implementation() const
{
	return EInteractionExecutionPolicy::LocalOnly;
}

void ACharacterSelectionPedestal::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
                                                       UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// 플레이어가 범위 안에 들어오면 상호작용 UI 표시
	if (APawn* Pawn = Cast<APawn>(OtherActor))
	{
		if (AERNPlayerController* PC = Cast<AERNPlayerController>(Pawn->GetController()))
		{
			PC->SetCurrentInteractable(this);

			// 상호작용 위젯 표시
			if (InteractionPromptWidget)
			{
				InteractionPromptWidget->SetVisibility(true);
			}
		}
	}
}

void ACharacterSelectionPedestal::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	// 플레이어가 범위 밖으로 나가면 상호작용 UI 숨김
	if (APawn* Pawn = Cast<APawn>(OtherActor))
	{
		if (AERNPlayerController* PC = Cast<AERNPlayerController>(Pawn->GetController()))
		{
			PC->ClearCurrentInteractable();

			// 상호작용 위젯 숨김
			if (InteractionPromptWidget)
			{
				InteractionPromptWidget->SetVisibility(false);
			}
		}
	}
}
