// Fill out your copyright notice in the Description page of Project Settings.

#include "Actors/Intro/ERNIntroBird.h"

#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "NiagaraComponent.h"
#include "Curves/CurveFloat.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "Character/Player/ProjectERNCharacter.h"

AERNIntroBird::AERNIntroBird()
{
	PrimaryActorTick.bCanEverTick = true;

	bReplicates = true;
	SetReplicateMovement(true);

	// Root
	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	// Bird Mesh
	BirdMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("BirdMesh"));
	BirdMesh->SetupAttachment(Root);
	BirdMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Hang Point
	HangPoint = CreateDefaultSubobject<USceneComponent>(TEXT("HangPoint"));
	HangPoint->SetupAttachment(BirdMesh);

	// Flight VFX (BP에서 NiagaraSystem 에셋 할당)
	FlightVFX = CreateDefaultSubobject<UNiagaraComponent>(TEXT("FlightVFX"));
	FlightVFX->SetupAttachment(BirdMesh);
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
}

void AERNIntroBird::StartFlight()
{
	if (!HasAuthority())
	{
		return;
	}

	StartLocation = GetActorLocation();
	FlightDirection = GetActorForwardVector();
	ElapsedTime = 0.f;
	bIsFlying = true;
}

void AERNIntroBird::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (!HasAuthority())
	{
		return;
	}

	if (bIsFlying)
	{
		ElapsedTime += DeltaTime;

		const float Alpha = FMath::Clamp(ElapsedTime / FMath::Max(FlightDuration, KINDA_SMALL_NUMBER), 0.f, 1.f);
		const float Progress = FlightCurve ? FlightCurve->GetFloatValue(Alpha) : Alpha;

		const FVector NewLocation = StartLocation + FlightDirection * (FlightDistance * Progress);
		SetActorLocation(NewLocation);

		// 비행 시간 끝나면 더 이상 이동 안 함 (그대로 정지 — 플레이어가 점프 누를 때까지 대기)
		if (Alpha >= 1.f)
		{
			bIsFlying = false;
		}
	}
	else if (bIsFlyingAway)
	{
		const FVector NewLocation = GetActorLocation() + FlyAwayDirection.GetSafeNormal() * (FlyAwaySpeed * DeltaTime);
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
	bIsFlyingAway = true;

	// 잔존 파티클 생성 중단 (이미 발생한 파티클은 자연 소멸)
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

void AERNIntroBird::DestroySelf()
{
	Destroy();
}
