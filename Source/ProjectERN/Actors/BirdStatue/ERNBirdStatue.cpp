// Copyright Epic Games, Inc. All Rights Reserved.

#include "Actors/BirdStatue/ERNBirdStatue.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Components/SceneComponent.h"
#include "NiagaraComponent.h"
#include "GameFramework/PlayerController.h"
#include "Character/Player/ERNPlayerController.h"
#include "Character/Player/ProjectERNCharacter.h"
#include "Actors/Intro/ERNIntroBird.h"

AERNBirdStatue::AERNBirdStatue()
{
	PrimaryActorTick.bCanEverTick = false;

	// Scene Root - StatueMesh를 자식으로 두면 BP에서 메시 회전 자유롭게 가능
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	RootComponent = SceneRoot;

	// 조각상 메시
	StatueMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StatueMesh"));
	StatueMesh->SetupAttachment(RootComponent);

	// 상호작용 범위
	InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionSphere"));
	InteractionSphere->SetupAttachment(RootComponent);
	InteractionSphere->SetSphereRadius(200.f);
	InteractionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	// 전 채널 Overlap → 캐릭터 InteractionDetector가 오브젝트 타입/캡슐 높이와 무관하게 이 구체를 탐지
	// (OnSphereBeginOverlap에서 Pawn만 필터링하므로 프롬프트 로직은 영향 없음)
	InteractionSphere->SetCollisionResponseToAllChannels(ECR_Overlap);

	// 상호작용 프롬프트 위젯
	InteractionPromptWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("InteractionPromptWidget"));
	InteractionPromptWidget->SetupAttachment(RootComponent);
	InteractionPromptWidget->SetWidgetSpace(EWidgetSpace::World);
	InteractionPromptWidget->SetVisibility(false);

	// 상시 이펙트 (BP에서 NiagaraSystem 에셋 할당)
	AmbientVFX = CreateDefaultSubobject<UNiagaraComponent>(TEXT("AmbientVFX"));
	AmbientVFX->SetupAttachment(StatueMesh);
	AmbientVFX->bAutoActivate = true;

	// 새 등장 위치 (BP에서 조각상 뒤쪽 위 배치)
	BirdSpawnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("BirdSpawnPoint"));
	BirdSpawnPoint->SetupAttachment(RootComponent);
	BirdSpawnPoint->SetRelativeLocation(FVector(-3000.f, 0.f, 5000.f));  // 기본값: 뒤로 -300, 위로 +500
}

void AERNBirdStatue::BeginPlay()
{
	Super::BeginPlay();

	InteractionSphere->OnComponentBeginOverlap.AddDynamic(this, &AERNBirdStatue::OnSphereBeginOverlap);
	InteractionSphere->OnComponentEndOverlap.AddDynamic(this, &AERNBirdStatue::OnSphereEndOverlap);
}

void AERNBirdStatue::Interact_Implementation(APlayerController* PlayerController)
{
	// 서버 권한 정책이므로 서버에서만 실행됨 (인터랙션 시스템이 RPC 처리)
	if (!HasAuthority() || !PlayerController || !BirdClass)
	{
		return;
	}

	AProjectERNCharacter* Player = Cast<AProjectERNCharacter>(PlayerController->GetPawn());
	if (!Player || Player->IsDead())
	{
		return;
	}

	// 이미 새를 호출했거나(Approach 중) 매달려 있으면 중복 사용 차단 — 하차 전까지 재입력 무시
	if (Player->IsBirdRideActive() || Player->GetAttachedBird())
	{
		return;
	}

	// 새 스폰 (BirdSpawnPoint 트랜스폼)
	const FTransform SpawnXform = BirdSpawnPoint
		? BirdSpawnPoint->GetComponentTransform()
		: GetActorTransform();

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AERNIntroBird* Bird = GetWorld()->SpawnActor<AERNIntroBird>(BirdClass, SpawnXform, SpawnParams);
	if (!Bird)
	{
		return;
	}

	// 이 동상의 비행 파라미터를 새에 주입 (Replicated → 클라까지 전파)
	Bird->ConfigureFlight(AscentHeight, AscentForwardDistance, FlightDistance, FlightDuration);

	// Approach → Ascend → Forward 시퀀스 시작
	// Ascend 방향은 조각상 Forward (수평으로 정규화)
	Bird->StartApproachAndPickup(Player, GetActorForwardVector());

	// 새 호출 성공 → 하차 전까지 재입력 차단
	Player->SetBirdRideActive(true);
}

bool AERNBirdStatue::CanInteract_Implementation() const
{
	// 무제한 사용
	return true;
}

FText AERNBirdStatue::GetInteractionText_Implementation() const
{
	return FText::FromString(TEXT("E키를 눌러 비행"));
}

EInteractionExecutionPolicy AERNBirdStatue::GetInteractionExecutionPolicy_Implementation() const
{
	// 서버 권한 - 새 스폰/부착은 서버에서 처리
	return EInteractionExecutionPolicy::ServerAuthority;
}

void AERNBirdStatue::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (APawn* Pawn = Cast<APawn>(OtherActor))
	{
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

void AERNBirdStatue::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (APawn* Pawn = Cast<APawn>(OtherActor))
	{
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
