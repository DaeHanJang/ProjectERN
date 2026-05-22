// Fill out your copyright notice in the Description page of Project Settings.

#include "Actors/Intro/ERNIntroBird.h"

#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/GameStateBase.h"
#include "NiagaraComponent.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "Character/Player/ProjectERNCharacter.h"

AERNIntroBird::AERNIntroBird()
{
	PrimaryActorTick.bCanEverTick = true;

	bReplicates = true;
	SetReplicateMovement(false);  // 자동 위치 리플 OFF — 모든 머신이 deterministic하게 동일 계산

	// 자동 possess 차단
	AutoPossessAI = EAutoPossessAI::Disabled;
	AutoPossessPlayer = EAutoReceiveInput::Disabled;

	// Capsule 콜리전 끔
	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	// Mesh 콜리전 끔
	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	// CMC는 안 씀 — 이동은 SetActorLocation 직접 사용
	if (UCharacterMovementComponent* CMC = GetCharacterMovement())
	{
		CMC->SetMovementMode(MOVE_Flying);
		CMC->GravityScale = 0.f;
		CMC->bOrientRotationToMovement = false;
		CMC->NetworkSmoothingMode = ENetworkSmoothingMode::Disabled;
	}

	// Hang Point
	HangPoint = CreateDefaultSubobject<USceneComponent>(TEXT("HangPoint"));
	HangPoint->SetupAttachment(GetMesh());

	// Flight VFX
	FlightVFX = CreateDefaultSubobject<UNiagaraComponent>(TEXT("FlightVFX"));
	FlightVFX->SetupAttachment(GetMesh());
	FlightVFX->bAutoActivate = true;
}

void AERNIntroBird::BeginPlay()
{
	Super::BeginPlay();
}

void AERNIntroBird::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AERNIntroBird, AttachedPlayer);
	DOREPLIFETIME(AERNIntroBird, bIsFlying);
	DOREPLIFETIME(AERNIntroBird, StartLocation);
	DOREPLIFETIME(AERNIntroBird, FlightDirection);
	DOREPLIFETIME(AERNIntroBird, FlightStartServerTime);
	DOREPLIFETIME(AERNIntroBird, bIsFlyingAway);
	DOREPLIFETIME(AERNIntroBird, FlyAwayStartLocation);
	DOREPLIFETIME(AERNIntroBird, FlyAwayStartServerTime);
}

float AERNIntroBird::GetServerNow() const
{
	if (const UWorld* World = GetWorld())
	{
		if (const AGameStateBase* GS = World->GetGameState())
		{
			return GS->GetServerWorldTimeSeconds();
		}
		return World->GetTimeSeconds();
	}
	return 0.f;
}

void AERNIntroBird::StartFlight()
{
	if (!HasAuthority())
	{
		return;
	}

	// 시작 정보를 Replicated 변수에 세팅 — initial replication 또는 즉시 리플로 모든 클라에 전파
	StartLocation = GetActorLocation();
	FlightDirection = GetActorForwardVector();
	FlightStartServerTime = GetServerNow();
	LocalFlightStartTime = GetWorld()->GetTimeSeconds();  // 서버 자기 머신의 로컬 시작 시각
	bIsFlying = true;
}

void AERNIntroBird::OnRep_IsFlying()
{
	if (bIsFlying)
	{
		// 클라가 받은 시점에 서버 기준으로 이미 경과한 시간만큼 LocalFlightStartTime을 과거로 보정
		// → 이후 Tick은 자기 GetTimeSeconds()로 일관 진행 (ServerWorldTimeSecondsDelta 변동 영향 차단)
		const float ServerElapsed = FMath::Max(0.f, GetServerNow() - FlightStartServerTime);
		LocalFlightStartTime = GetWorld()->GetTimeSeconds() - ServerElapsed;
	}
}

void AERNIntroBird::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// 모든 머신이 자기 로컬 시간 기반으로 일관 계산 (시작 시 한 번 lag 보정 후 자체 진행)
	if (bIsFlying)
	{
		const float ElapsedTime = FMath::Max(0.f, GetWorld()->GetTimeSeconds() - LocalFlightStartTime);
		const float Alpha = FMath::Clamp(ElapsedTime / FMath::Max(FlightDuration, KINDA_SMALL_NUMBER), 0.f, 1.f);
		const FVector NewLocation = StartLocation + FlightDirection * (FlightDistance * Alpha);
		SetActorLocation(NewLocation);

		// 비행 종료 처리는 서버만 (Replicated bool로 클라에 전파)
		if (HasAuthority() && Alpha >= 1.f)
		{
			bIsFlying = false;
		}
	}
	else if (bIsFlyingAway)
	{
		const float ElapsedTime = FMath::Max(0.f, GetWorld()->GetTimeSeconds() - LocalFlyAwayStartTime);
		const FVector NewLocation = FlyAwayStartLocation + FlyAwayDirection.GetSafeNormal() * (FlyAwaySpeed * ElapsedTime);
		SetActorLocation(NewLocation);
	}
}

void AERNIntroBird::OnPlayerReleased()
{
	if (!HasAuthority())
	{
		return;
	}

	AttachedPlayer = nullptr;
	bIsFlying = false;

	// FlyAway 시작 정보 세팅 (Replicated → 클라가 OnRep_IsFlyingAway에서 VFX 끄기 + lag 보정)
	FlyAwayStartLocation = GetActorLocation();
	FlyAwayStartServerTime = GetServerNow();
	LocalFlyAwayStartTime = GetWorld()->GetTimeSeconds();
	bIsFlyingAway = true;

	// 서버 측에서도 VFX 끄기 (OnRep은 클라만 실행됨)
	if (FlightVFX)
	{
		FlightVFX->Deactivate();
	}

	// FlyAwayDuration 후 Destroy
	GetWorldTimerManager().SetTimer(
		FlyAwayTimerHandle,
		this,
		&AERNIntroBird::DestroySelf,
		FlyAwayDuration,
		false
	);
}

void AERNIntroBird::OnRep_IsFlyingAway()
{
	if (bIsFlyingAway)
	{
		// FlyAway도 lag 보정
		const float ServerElapsed = FMath::Max(0.f, GetServerNow() - FlyAwayStartServerTime);
		LocalFlyAwayStartTime = GetWorld()->GetTimeSeconds() - ServerElapsed;

		if (FlightVFX)
		{
			FlightVFX->Deactivate();
		}
	}
}

void AERNIntroBird::DestroySelf()
{
	Destroy();
}
