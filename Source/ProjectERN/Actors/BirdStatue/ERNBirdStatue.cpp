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
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

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

	SetActorTickEnabled(true);

	ComponentBaseZMap.Empty();
	
	// 블루프린트에서 임의로 추가한 Wing 컴포넌트 등 모든 스태틱 메시를 찾아 등록
	TArray<UStaticMeshComponent*> MeshComps;
	GetComponents<UStaticMeshComponent>(MeshComps);
	for (UStaticMeshComponent* Mesh : MeshComps)
	{
		if (Mesh)
		{
			ComponentBaseZMap.Add(Mesh, Mesh->GetRelativeLocation().Z);
		}
	}

	InteractionSphere->OnComponentBeginOverlap.AddUniqueDynamic(this, &AERNBirdStatue::OnSphereBeginOverlap);
	InteractionSphere->OnComponentEndOverlap.AddUniqueDynamic(this, &AERNBirdStatue::OnSphereEndOverlap);
}

void AERNBirdStatue::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (ComponentBaseZMap.Num() > 0)
	{
		RunningTime += DeltaTime;
		float DeltaZ = FMath::Sin(RunningTime * BobSpeed) * BobHeight;
		
		for (auto& Pair : ComponentBaseZMap)
		{
			if (USceneComponent* Comp = Pair.Key)
			{
				FVector NewLocation = Comp->GetRelativeLocation();
				NewLocation.Z = Pair.Value + DeltaZ;
				Comp->SetRelativeLocation(NewLocation);
			}
		}
	}
}

void AERNBirdStatue::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (InteractionSphere)
	{
		InteractionSphere->OnComponentBeginOverlap.RemoveDynamic(this, &AERNBirdStatue::OnSphereBeginOverlap);
		InteractionSphere->OnComponentEndOverlap.RemoveDynamic(this, &AERNBirdStatue::OnSphereEndOverlap);
	}
	
	
	Super::EndPlay(EndPlayReason);
}

void AERNBirdStatue::Interact_Implementation(APlayerController* PlayerController)
{
	// 서버 권한 정책이므로 서버에서만 실행됨 (인터랙션 시스템이 RPC 처리)
	if (!HasAuthority() || !PlayerController)
	{
		return;
	}

	AProjectERNCharacter* Player = Cast<AProjectERNCharacter>(PlayerController->GetPawn());
	if (!Player)
	{
		return;
	}

	// 새 스폰 위치 (BirdSpawnPoint 트랜스폼)
	const FTransform SpawnXform = BirdSpawnPoint
		? BirdSpawnPoint->GetComponentTransform()
		: GetActorTransform();

	// 가드/스폰/Approach/재입력차단 공용 헬퍼. Ascend 방향은 조각상 Forward(수평 정규화).
	// bConsoleSummon=false → 동상 기본(주입값) 비행.
	AERNIntroBird* Bird = AERNIntroBird::RequestPickup(
		BirdClass, Player, SpawnXform, GetActorForwardVector(), /*bConsoleSummon=*/false);

	if (Bird)
	{
		// 이 동상의 개별 비행 파라미터 주입 (Replicated → 클라까지 전파)
		Bird->ConfigureFlight(AscentHeight, AscentForwardDistance, FlightDistance, FlightDuration);
		
		AERNCharacterBase* Character = Cast<AERNCharacterBase>(PlayerController->GetPawn());
		if (Character)
		{
			Character->Multicast_PlayEffectAndSound(nullptr, FVector::ZeroVector, Sound, Character->GetActorLocation());
		}
	}
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
		// 로컬 플레이어만 — 서버가 원격 클라 폰 오버랩으로 호스트 화면에 띄우는 것 방지
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

void AERNBirdStatue::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	if (APawn* Pawn = Cast<APawn>(OtherActor))
	{
		// 로컬 플레이어만 — 서버가 원격 클라 폰 오버랩으로 호스트 화면에 띄우는 것 방지
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
