// Copyright Epic Games, Inc. All Rights Reserved.

#include "Actors/Portal/ERNInstancePortal.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/WidgetComponent.h"
#include "NiagaraComponent.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/Controller.h"
#include "Character/Player/ERNPlayerController.h"

AERNInstancePortal::AERNInstancePortal()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	PortalMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PortalMesh"));
	PortalMesh->SetupAttachment(RootComponent);
	PortalMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionSphere"));
	InteractionSphere->SetupAttachment(RootComponent);
	InteractionSphere->SetSphereRadius(200.f);
	InteractionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	// 전 채널 Overlap → 캐릭터 InteractionDetector가 이 구체를 탐지 (BirdStatue와 동일)
	InteractionSphere->SetCollisionResponseToAllChannels(ECR_Overlap);

	InteractionPromptWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("InteractionPromptWidget"));
	InteractionPromptWidget->SetupAttachment(RootComponent);
	InteractionPromptWidget->SetWidgetSpace(EWidgetSpace::World);
	InteractionPromptWidget->SetVisibility(false);

	PortalVFX = CreateDefaultSubobject<UNiagaraComponent>(TEXT("PortalVFX"));
	PortalVFX->SetupAttachment(RootComponent);
	PortalVFX->bAutoActivate = true;
}

void AERNInstancePortal::BeginPlay()
{
	Super::BeginPlay();

	InteractionSphere->OnComponentBeginOverlap.AddUniqueDynamic(this, &AERNInstancePortal::OnSphereBeginOverlap);
	InteractionSphere->OnComponentEndOverlap.AddUniqueDynamic(this, &AERNInstancePortal::OnSphereEndOverlap);
}

void AERNInstancePortal::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (InteractionSphere)
	{
		InteractionSphere->OnComponentBeginOverlap.RemoveDynamic(this, &AERNInstancePortal::OnSphereBeginOverlap);
		InteractionSphere->OnComponentEndOverlap.RemoveDynamic(this, &AERNInstancePortal::OnSphereEndOverlap);
	}

	Super::EndPlay(EndPlayReason);
}

void AERNInstancePortal::Interact_Implementation(APlayerController* PlayerController)
{
	UE_LOG(LogTemp, Warning, TEXT("[InstancePortal] Interact 진입 - HasAuthority=%d"), HasAuthority());

	// 서버에서만 실행
	if (!HasAuthority())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Warning, TEXT("[InstancePortal] World is null"));
		return;
	}

	AGameStateBase* GS = World->GetGameState();
	if (!GS)
	{
		UE_LOG(LogTemp, Warning, TEXT("[InstancePortal] GameState is null"));
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[InstancePortal] PlayerArray=%d, DestPoints=%d"),
		GS->PlayerArray.Num(), DestinationPoints.Num());

	// 한 명이 눌러도 전원 이동
	int32 Index = 0;
	for (APlayerState* PS : GS->PlayerArray)
	{
		APawn* Pawn = PS ? PS->GetPawn() : nullptr;
		if (!Pawn)
		{
			UE_LOG(LogTemp, Warning, TEXT("[InstancePortal] PS=%s Pawn is null"), *GetNameSafe(PS));
			continue;
		}

		const FTransform Dest = ResolveDestination(Index++);

		UE_LOG(LogTemp, Warning, TEXT("[InstancePortal] %s : %s -> %s"),
			*Pawn->GetName(), *Pawn->GetActorLocation().ToString(), *Dest.GetLocation().ToString());

		// 서버에서 옮기면 RepMovement로 모든 클라에 복제
		const bool bMoved = Pawn->SetActorLocationAndRotation(Dest.GetLocation(), Dest.GetRotation(),
			false, nullptr, ETeleportType::TeleportPhysics);

		UE_LOG(LogTemp, Warning, TEXT("[InstancePortal] %s bMoved=%d, after=%s"),
			*Pawn->GetName(), bMoved, *Pawn->GetActorLocation().ToString());

		if (AController* C = Pawn->GetController())
		{
			C->SetControlRotation(Dest.GetRotation().Rotator());
		}
	}

	// 포탈별 채팅 알림 (입장/탈출 등)
	BroadcastPortalChat();

	// TODO: 자기장/낮밤 Pause(던전 입장) / Resume(필드 복귀) 연동
}

bool AERNInstancePortal::CanInteract_Implementation() const
{
	return true;
}

FText AERNInstancePortal::GetInteractionText_Implementation() const
{
	return PromptText;
}

EInteractionExecutionPolicy AERNInstancePortal::GetInteractionExecutionPolicy_Implementation() const
{
	// 전원 이동은 서버 권한
	return EInteractionExecutionPolicy::ServerAuthority;
}

FTransform AERNInstancePortal::ResolveDestination(int32 PlayerIndex) const
{
	// 지정된 도착 지점이 충분하면 해당 지점 그대로 사용
	if (DestinationPoints.IsValidIndex(PlayerIndex) && DestinationPoints[PlayerIndex])
	{
		return DestinationPoints[PlayerIndex]->GetActorTransform();
	}

	// 모자라면 기준 트랜스폼(첫 지점 또는 포탈) 주위로 원형 분산
	FTransform Base = (DestinationPoints.Num() > 0 && DestinationPoints[0])
		? DestinationPoints[0]->GetActorTransform()
		: GetActorTransform();

	if (DestinationSpreadRadius > 0.f)
	{
		// 인덱스별로 원 둘레에 고르게 배치 (겹침 방지)
		const float AngleStep = 137.5f;
		const float AngleDeg = AngleStep * PlayerIndex;
		const float Rad = FMath::DegreesToRadians(AngleDeg);
		const FVector Offset(FMath::Cos(Rad) * DestinationSpreadRadius,
			FMath::Sin(Rad) * DestinationSpreadRadius, 0.f);
		Base.AddToTranslation(Offset);
	}

	return Base;
}

void AERNInstancePortal::BroadcastPortalChat() const
{
	if (ChatMessage.IsEmpty())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		if (AERNPlayerController* PC = Cast<AERNPlayerController>(It->Get()))
		{
			PC->Client_ReceiveChat(ChatSender, ChatMessage, ChatColor);
		}
	}
}

void AERNInstancePortal::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (APawn* Pawn = Cast<APawn>(OtherActor))
	{
		// 로컬 플레이어만
		if (!Pawn->IsLocallyControlled())
		{
			return;
		}

		if (AERNPlayerController* PC = Cast<AERNPlayerController>(Pawn->GetController()))
		{
			PC->SetCurrentInteractable(this);

			if (InteractionPromptWidget)
			{
				InteractionPromptWidget->SetVisibility(true);
			}
		}
	}
}

void AERNInstancePortal::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (APawn* Pawn = Cast<APawn>(OtherActor))
	{
		// 로컬 플레이어만
		if (!Pawn->IsLocallyControlled())
		{
			return;
		}

		if (AERNPlayerController* PC = Cast<AERNPlayerController>(Pawn->GetController()))
		{
			PC->ClearCurrentInteractable();

			if (InteractionPromptWidget)
			{
				InteractionPromptWidget->SetVisibility(false);
			}
		}
	}
}
