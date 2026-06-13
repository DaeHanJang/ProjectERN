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
#include "Character/Player/ERNPlayerState.h"
#include "Core/ERNGameState.h"
#include "World/NightRainZoneManager.h"
#include "Actors/Portal/ERNPortalDestinationPoint.h"
#include "EngineUtils.h"
#include "WorldPartition/DataLayer/DataLayerManager.h"

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

void AERNInstancePortal::SetDestinationPoint(AERNPortalDestinationPoint* InDestinationPoint)
{
	if (!HasAuthority() || InDestinationPoint == nullptr)
	{
		return;
	}

	// 동적 스폰 포탈은 단일 도착 지점을 사용 (전원은 ResolveDestination의 분산 로직으로 흩어짐)
	DestinationPoints.Reset();
	DestinationPoints.Add(InDestinationPoint);
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

	AERNGameState* GS = World->GetGameState<AERNGameState>();
	if (!GS)
	{
		UE_LOG(LogTemp, Warning, TEXT("[InstancePortal] GameState is null"));
		return;
	}
	
	// 전원 함께 이동하므로 누른 플레이어의 상태로 입장/복귀를 일괄 판정 (엇갈림 방지)
	bool bEntering = true;
	if (const AERNPlayerState* InteractorPS = PlayerController ? PlayerController->GetPlayerState<AERNPlayerState>() : nullptr)
	{
		bEntering = (InteractorPS->bIsInInstance == false);
	}

	// 동적 스폰 포탈은 에디터 참조가 없으므로 런타임에 확보
	ANightRainZoneManager* ZoneManager = ResolveNightRainZoneManager();
	
	for (APlayerState* PS : GS->PlayerArray)
	{
		APawn* Pawn = PS ? PS->GetPawn() : nullptr;
		if (!Pawn)
		{
			continue;
		}
		
		if (AERNPlayerController* ERNPC = Cast<AERNPlayerController>(Pawn->GetController()))
		{
			ERNPC->Client_CloseInteractableWidgetsForPortal();
		}
	}

	// 한 명이 눌러도 전원 이동
	int32 Index = 0;
	for (APlayerState* PS : GS->PlayerArray)
	{
		APawn* Pawn = PS ? PS->GetPawn() : nullptr;
		if (!Pawn)
		{
			continue;
		}

		AERNPlayerState* ERNPlayerState = Cast<AERNPlayerState>(PS);
		if (ERNPlayerState == nullptr)
		{
			continue;
		}
		
		// 개별 플레이어 자기장 무적 적용
		if (ZoneManager)
		{
			if (AERNPlayerController* ERNPC = Cast<AERNPlayerController>(Pawn->GetController()))
			{
				ZoneManager->SetIgnoreNightRainZone(ERNPC, bEntering);
			}
		}
		
		
		FTransform Dest;
		if (bEntering)
		{
			// 입장 - 데이터 레이어 활성화
			if (SetDungeonDataLayerActive(true) == false)
			{
				UE_LOG(LogTemp,Warning, TEXT("던전 활성화 실패. 텔레포트를 시작하지 않습니다."));
				return;
			}
			
			// 입장 — 현재 필드 위치를 각자 저장한 뒤 도착 지점으로 이동
			ERNPlayerState->SavedFieldTransform = Pawn->GetActorTransform();
			Dest = ResolveDestination(Index++);
		}
		else
		{
			// 복귀 — 저장해둔 필드 위치로 순간이동 (저장하지 않음)
			Dest = ERNPlayerState->SavedFieldTransform;
			
			// 복귀 - 데이터 레이어 비활성화
			if (SetDungeonDataLayerActive(false) == false)
			{
				// 복귀는 데이터 레이어 관리와 상관없이 가능하도록
				UE_LOG(LogTemp, Warning, TEXT("던전 비활성화 실패"));
			}
		}

		// 서버에서 옮기면 RepMovement로 모든 클라에 복제
		Pawn->SetActorLocationAndRotation(Dest.GetLocation(), Dest.GetRotation(),
			false, nullptr, ETeleportType::TeleportPhysics);

		if (AController* C = Pawn->GetController())
		{
			C->SetControlRotation(Dest.GetRotation().Rotator());
		}

		// 상태 토글 + 입장 인원 카운트 갱신 (자기장 Pause 판단용)
		ERNPlayerState->bIsInInstance = bEntering;
		if (bEntering)
		{
			GS->AddInstancePortalState(ERNPlayerState);
		}
		else
		{
			GS->RemoveInstancePortalState(ERNPlayerState);
		}
	}

	// 자기장 진행 Pause(입장) / Resume(복귀) — 전원 처리 후 한 번만
	if (ZoneManager)
	{
		if (bEntering && ZoneManager->bIsPauseZone() == false)
		{
			ZoneManager->PauseZoneProgress_ServerOnly();
		}
		else if (!bEntering && ZoneManager->bIsPauseZone() == true)
		{
			ZoneManager->ResumeZoneProgress_ServerOnly();
		}
	}

	// 포탈별 채팅 알림 (입장/복귀에 따라 다른 문구)
	BroadcastPortalChat(bEntering);
	
	if (!bIsreusable)
	{
		Destroy();
	}
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

ANightRainZoneManager* AERNInstancePortal::ResolveNightRainZoneManager()
{
	// 에디터에서 배치된 참조가 있으면 그대로 사용
	if (IsValid(NightRainZoneManager))
	{
		return NightRainZoneManager;
	}

	// 동적 스폰 포탈 등 참조가 비어있으면 월드에서 탐색하여 캐싱
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return nullptr;
	}

	for (TActorIterator<ANightRainZoneManager> It(World); It; ++It)
	{
		if (IsValid(*It))
		{
			NightRainZoneManager = *It;
			return NightRainZoneManager;
		}
	}

	return nullptr;
}


void AERNInstancePortal::BroadcastPortalChat(bool bEntering) const
{
	// 입장/복귀에 따라 다른 문구 선택
	const FString& Message = bEntering ? EnterChatMessage : ReturnChatMessage;
	if (Message.IsEmpty())
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
			PC->Client_ReceiveChat(ChatSender, Message, ChatColor);
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


bool AERNInstancePortal::SetDungeonDataLayerActive(bool bActive) const
{
	if (HasAuthority() == false || DungeonDataLayerAsset == nullptr)
	{
		return false;
	}
	
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return false;
	}
	
	UDataLayerManager* DataLayerManager = World->GetDataLayerManager();
	if (DataLayerManager == nullptr)
	{
		return false;
	}
	
	// 런타임 중 데이터 레이어 활성화
	if (bActive)
	{
		return DataLayerManager->SetDataLayerRuntimeState(DungeonDataLayerAsset, EDataLayerRuntimeState::Activated, true);
	}
	// 런타임 중 데이터 레이어 비활성화
	else
	{
		return DataLayerManager->SetDataLayerRuntimeState(DungeonDataLayerAsset, EDataLayerRuntimeState::Unloaded, true);
	}
}
