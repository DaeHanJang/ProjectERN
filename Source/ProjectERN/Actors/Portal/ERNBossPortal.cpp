// Fill out your copyright notice in the Description page of Project Settings.

#include "Actors/Portal/ERNBossPortal.h"

#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "NiagaraComponent.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "Character/Player/ProjectERNCharacter.h"
#include "Character/Player/ERNPlayerState.h"
#include "Subsystem/ERNCutsceneSubsystem.h"

AERNBossPortal::AERNBossPortal()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	// Root: BoxComponent (트리거 볼륨)
	TriggerVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerVolume"));
	SetRootComponent(TriggerVolume);
	TriggerVolume->SetBoxExtent(FVector(150.f, 150.f, 150.f));
	TriggerVolume->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	TriggerVolume->SetGenerateOverlapEvents(true);

	// 시각 메시
	PortalMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PortalMesh"));
	PortalMesh->SetupAttachment(TriggerVolume);
	PortalMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// VFX
	PortalVFX = CreateDefaultSubobject<UNiagaraComponent>(TEXT("PortalVFX"));
	PortalVFX->SetupAttachment(TriggerVolume);
	PortalVFX->bAutoActivate = true;
}

void AERNBossPortal::BeginPlay()
{
	Super::BeginPlay();

	// 오버랩 이벤트는 서버에서만 처리
	if (HasAuthority() && TriggerVolume)
	{
		TriggerVolume->OnComponentBeginOverlap.AddDynamic(this, &AERNBossPortal::OnTriggerBeginOverlap);
		TriggerVolume->OnComponentEndOverlap.AddDynamic(this, &AERNBossPortal::OnTriggerEndOverlap);
	}
}

void AERNBossPortal::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AERNBossPortal, ReadyCount);
}

void AERNBossPortal::OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority() || bTravelTriggered)
	{
		return;
	}

	AProjectERNCharacter* Player = Cast<AProjectERNCharacter>(OtherActor);
	if (!Player)
	{
		return;
	}

	// 이미 등록되어 있으면 무시
	if (OverlappingPlayers.ContainsByPredicate([Player](const TWeakObjectPtr<AProjectERNCharacter>& Weak)
	{
		return Weak.Get() == Player;
	}))
	{
		return;
	}

	OverlappingPlayers.Add(Player);
	ReadyCount = OverlappingPlayers.Num();

	CheckAndTriggerTravel();
}

void AERNBossPortal::OnTriggerEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (!HasAuthority() || bTravelTriggered)
	{
		return;
	}

	AProjectERNCharacter* Player = Cast<AProjectERNCharacter>(OtherActor);
	if (!Player)
	{
		return;
	}

	OverlappingPlayers.RemoveAll([Player](const TWeakObjectPtr<AProjectERNCharacter>& Weak)
	{
		return !Weak.IsValid() || Weak.Get() == Player;
	});

	ReadyCount = OverlappingPlayers.Num();
}

// 모든 플레이어 체크 후 맵 이동
void AERNBossPortal::CheckAndTriggerTravel()
{
	if (bTravelTriggered) return;

	UWorld* World = GetWorld();
	if (!World) return;

	AGameStateBase* GS = World->GetGameState();
	if (!GS) return;

	const int32 TotalPlayers = GS->PlayerArray.Num();
	if (TotalPlayers <= 0) return;

	// 만료된 weak ref 정리
	OverlappingPlayers.RemoveAll([](const TWeakObjectPtr<AProjectERNCharacter>& Weak)
	{
		return !Weak.IsValid();
	});
	ReadyCount = OverlappingPlayers.Num();

	if (OverlappingPlayers.Num() < TotalPlayers)
	{
		return;
	}

	// 조건 충족 — 트래블 시작
	bTravelTriggered = true;

	UE_LOG(LogTemp, Log, TEXT("[BossPortal] All %d players ready, traveling to %s"),
		TotalPlayers, *BossMapName);

	// 모든 클라이언트에 로딩 화면 표시
	Multicast_ShowLoadingScreen();

	// 필드→보스: 각 플레이어 진행 상황(레벨/골드/인벤/무기) 스냅샷 저장 → 보스맵에서 복원
	for (APlayerState* PS : GS->PlayerArray)
	{
		if (AERNPlayerState* ERNPS = Cast<AERNPlayerState>(PS))
		{
			ERNPS->SaveSnapshotFromPawn();
		}
	}

	// 서버 트래블
	World->ServerTravel(BossMapName + TEXT("?listen"));
}

void AERNBossPortal::Multicast_ShowLoadingScreen_Implementation()
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UERNCutsceneSubsystem* CutsceneSubsystem = GI->GetSubsystem<UERNCutsceneSubsystem>())
		{
			// 포탈에 전용 위젯이 할당돼 있으면 그걸, 없으면 내부에서 기본 위젯으로 폴백
			CutsceneSubsystem->ShowLoadingScreen(LoadingWidgetOverride);
		}
	}
}
