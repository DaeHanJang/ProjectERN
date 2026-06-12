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
#include "Components/AudioComponent.h"

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

	// Flight Wind Audio
	FlightWindAudio = CreateDefaultSubobject<UAudioComponent>(TEXT("FlightWindAudio"));
	FlightWindAudio->SetupAttachment(GetMesh());
	FlightWindAudio->bAutoActivate = false;
}

void AERNIntroBird::BeginPlay()
{
	Super::BeginPlay();

	// HangBoneName 지정 시 본의 rest 위치 + HangPoint의 BP 위치를 캐싱 → Tick에서 본 delta 적용
	if (HangPoint && GetMesh() && !HangBoneName.IsNone())
	{
		HangPointBaseRelative = HangPoint->GetRelativeLocation();
		BoneRestComponentLoc = GetMesh()->GetBoneLocation(HangBoneName, EBoneSpaces::ComponentSpace);
		bHangBoneTracking = true;
	}
}

void AERNIntroBird::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AERNIntroBird, AttachedPlayer);
	DOREPLIFETIME(AERNIntroBird, bConsoleSummon);
	DOREPLIFETIME(AERNIntroBird, bIsFlying);
	DOREPLIFETIME(AERNIntroBird, StartLocation);
	DOREPLIFETIME(AERNIntroBird, FlightDirection);
	DOREPLIFETIME(AERNIntroBird, FlightStartServerTime);
	DOREPLIFETIME(AERNIntroBird, bIsFlyingAway);
	DOREPLIFETIME(AERNIntroBird, FlyAwayStartLocation);
	DOREPLIFETIME(AERNIntroBird, FlyAwayStartServerTime);
	DOREPLIFETIME(AERNIntroBird, CurrentSteeringOffset);
	// Approach / Ascend (BirdStatue 페이즈)
	DOREPLIFETIME(AERNIntroBird, bIsApproaching);
	DOREPLIFETIME(AERNIntroBird, ApproachTarget);
	DOREPLIFETIME(AERNIntroBird, bIsAscending);
	DOREPLIFETIME(AERNIntroBird, AscentStartLocation);
	DOREPLIFETIME(AERNIntroBird, AscentDirection);
	DOREPLIFETIME(AERNIntroBird, AscentStartServerTime);
	// BirdStatue 인스턴스별 비행 파라미터 (deterministic 시뮬 일치용)
	DOREPLIFETIME(AERNIntroBird, AscentHeight);
	DOREPLIFETIME(AERNIntroBird, AscentForwardDistance);
	DOREPLIFETIME(AERNIntroBird, FlightDistance);
	DOREPLIFETIME(AERNIntroBird, FlightDuration);
}

void AERNIntroBird::Server_SetSteeringInput_Implementation(float Input)
{
	CurrentSteeringInput = FMath::Clamp(Input, -1.f, 1.f);
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

	// 리슨 서버인 경우 직접 오디오 재생
	if (GetNetMode() != NM_DedicatedServer && FlightWindAudio)
	{
		FlightWindAudio->Play();
	}
}

void AERNIntroBird::StartApproachAndPickup(AProjectERNCharacter* Target, FVector InAscentDirection)
{
	if (!HasAuthority() || !Target)
	{
		return;
	}

	ApproachTarget = Target;

	// Ascend 시 사용할 방향 미리 저장 (Z 성분 제거, 수평 정규화)
	FVector Dir = InAscentDirection;
	Dir.Z = 0.f;
	AscentDirection = Dir.IsNearlyZero() ? FVector::ForwardVector : Dir.GetSafeNormal();

	bCameraPrewarmTriggered = false;
	bIsApproaching = true;
}

AERNIntroBird* AERNIntroBird::RequestPickup(TSubclassOf<AERNIntroBird> BirdClass,
	AProjectERNCharacter* Target, const FTransform& SpawnXform, FVector Direction, bool bConsoleSummon)
{
	if (!Target || !BirdClass || !Target->HasAuthority())
	{
		return nullptr;
	}

	// 사망 / 이미 새 호출 중 / 매달림 → 중복 차단
	if (Target->IsDead() || Target->IsBirdRideActive() || Target->GetAttachedBird())
	{
		return nullptr;
	}

	UWorld* World = Target->GetWorld();
	if (!World)
	{
		return nullptr;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AERNIntroBird* Bird = World->SpawnActor<AERNIntroBird>(BirdClass, SpawnXform, SpawnParams);
	if (!Bird)
	{
		return nullptr;
	}

	Bird->bConsoleSummon = bConsoleSummon;             // 비행 거리/시간 분기 신호
	Bird->StartApproachAndPickup(Target, Direction);   // Approach → Ascend → 전진
	Target->SetBirdRideActive(true);                   // 하차 전까지 재입력 차단
	return Bird;
}

void AERNIntroBird::ConfigureFlight(float InAscentHeight, float InAscentForwardDistance, float InFlightDistance, float InFlightDuration)
{
	if (!HasAuthority())
	{
		return;
	}

	AscentHeight = InAscentHeight;
	AscentForwardDistance = InAscentForwardDistance;
	FlightDistance = InFlightDistance;
	FlightDuration = InFlightDuration;
}

void AERNIntroBird::OnRep_IsApproaching()
{
	// 클라는 자기 머신에서 ApproachTarget 위치 보고 직접 보간 (deterministic 아님이지만 짧은 페이즈)
}

void AERNIntroBird::OnApproachArrived()
{
	if (!HasAuthority())
	{
		return;
	}

	bIsApproaching = false;

	// 플레이어 부착 (기존 시스템) - 매달림 몽타주/소유권/HangPoint 추적 모두 처리됨
	if (ApproachTarget)
	{
		ApproachTarget->AttachToIntroBird(this);
	}

	// Ascend 페이즈 진입
	AscentStartLocation = GetActorLocation();
	AscentStartServerTime = GetServerNow();
	LocalAscentStartTime = GetWorld()->GetTimeSeconds();
	bIsAscending = true;
}

void AERNIntroBird::OnRep_IsAscending()
{
	if (bIsAscending)
	{
		// Lag 보정: 서버 기준 이미 경과한 시간만큼 LocalAscentStartTime을 과거로 보정
		const float ServerElapsed = FMath::Max(0.f, GetServerNow() - AscentStartServerTime);
		LocalAscentStartTime = GetWorld()->GetTimeSeconds() - ServerElapsed;
	}
}

void AERNIntroBird::OnAscentComplete()
{
	if (!HasAuthority())
	{
		return;
	}

	bIsAscending = false;

	// 솟구침 끝 → 기존 전진 비행으로 전환 (현재 위치/방향 기준)
	// 새의 방향을 AscentDirection으로 맞춰서 자연스럽게 forward 비행 진입
	const FRotator NewRot = AscentDirection.Rotation();
	SetActorRotation(NewRot);

	StartFlight();
}

void AERNIntroBird::OnRep_IsFlying()
{
	if (bIsFlying)
	{
		// 클라가 받은 시점에 서버 기준으로 이미 경과한 시간만큼 LocalFlightStartTime을 과거로 보정
		// → 이후 Tick은 자기 GetTimeSeconds()로 일관 진행 (ServerWorldTimeSecondsDelta 변동 영향 차단)
		const float ServerElapsed = FMath::Max(0.f, GetServerNow() - FlightStartServerTime);
		LocalFlightStartTime = GetWorld()->GetTimeSeconds() - ServerElapsed;

		if (FlightWindAudio)
		{
			FlightWindAudio->Play();
		}
	}
}

void AERNIntroBird::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// HangPoint 본 트래킹: rest로부터의 본 delta만 HangPoint에 추가 (BP 설정 위치는 유지)
	if (bHangBoneTracking && HangPoint && GetMesh())
	{
		const FVector BoneCur = GetMesh()->GetBoneLocation(HangBoneName, EBoneSpaces::ComponentSpace);
		HangPoint->SetRelativeLocation(HangPointBaseRelative + (BoneCur - BoneRestComponentLoc));
	}

	// === Approach 페이즈 (BirdStatue 전용) ===
	if (bIsApproaching)
	{
		if (!ApproachTarget || ApproachTarget->IsDead())
		{
			// 타겟 사라짐/사망 → 비행 취소 + Destroy
			if (HasAuthority())
			{
				// 부착 전 취소이므로 타겟의 재입력 차단 플래그 해제
				if (ApproachTarget)
				{
					ApproachTarget->SetBirdRideActive(false);
				}
				bIsApproaching = false;
				Destroy();
			}
			return;
		}

		// 목표 = 플레이어 정수리 위 (HangPoint가 자연스럽게 닿도록)
		const FVector TargetLoc = ApproachTarget->GetActorLocation() + FVector(0.f, 0.f, ApproachOverheadOffset);
		const FVector CurLoc = GetActorLocation();
		const FVector ToTarget = TargetLoc - CurLoc;
		const float DistRemaining = ToTarget.Size();

		// 이동 (속도 기반 보간)
		const float StepDist = ApproachSpeed * DeltaTime;
		const FVector NewLoc = (DistRemaining <= StepDist)
			? TargetLoc
			: CurLoc + ToTarget.GetSafeNormal() * StepDist;
		SetActorLocation(NewLoc);

		// 서버: HangPoint ↔ Player 거리 체크 → 닿으면 attach + Ascend 전환
		if (HasAuthority() && HangPoint)
		{
			const float HangToPlayer = FVector::Dist(
				HangPoint->GetComponentLocation(),
				ApproachTarget->GetActorLocation());

			// 카메라 prewarm: attach까지 남은 거리 / 속도 = ETA. ETA <= LeadTime 이면 1회 발사.
			if (!bCameraPrewarmTriggered)
			{
				const float DistToGrab = FMath::Max(0.f, HangToPlayer - AttachTriggerDistance);
				const float ETA = DistToGrab / FMath::Max(ApproachSpeed, KINDA_SMALL_NUMBER);
				if (ETA <= CameraPrewarmLeadTime)
				{
					ApproachTarget->Client_PrewarmHangingCamera();
					bCameraPrewarmTriggered = true;
				}
			}

			if (HangToPlayer <= AttachTriggerDistance)
			{
				OnApproachArrived();
			}
		}
		return;
	}

	// === Ascend 페이즈 (BirdStatue 전용) ===
	if (bIsAscending)
	{
		const float ElapsedTime = FMath::Max(0.f, GetWorld()->GetTimeSeconds() - LocalAscentStartTime);
		const float Alpha = FMath::Clamp(ElapsedTime / FMath::Max(AscentDuration, KINDA_SMALL_NUMBER), 0.f, 1.f);
		const float HeightAlpha = AscentHeightCurve ? AscentHeightCurve->GetFloatValue(Alpha) : Alpha;

		const FVector NewLocation = AscentStartLocation
			+ FVector(0.f, 0.f, AscentHeight * HeightAlpha)
			+ AscentDirection * (AscentForwardDistance * Alpha);
		SetActorLocation(NewLocation);

		if (HasAuthority() && Alpha >= 1.f)
		{
			OnAscentComplete();
		}
		return;
	}

	// 모든 머신이 자기 로컬 시간 기반으로 일관 계산 (시작 시 한 번 lag 보정 후 자체 진행)
	if (bIsFlying)
	{
		// 서버: 키 누르고 있는 동안 좌우 입력 누적 (떼면 그 위치 유지, 한계 없음)
		if (HasAuthority() && !FMath::IsNearlyZero(CurrentSteeringInput))
		{
			CurrentSteeringOffset += CurrentSteeringInput * SteeringSpeed * DeltaTime;
		}

		// 콘솔 소환이면 빠른 거리/시간 프로파일, 아니면 기본(동상 주입값)
		const float EffectiveDistance = bConsoleSummon ? ConsoleFlightDistance : FlightDistance;
		const float EffectiveDuration = bConsoleSummon ? ConsoleFlightDuration : FlightDuration;

		const float ElapsedTime = FMath::Max(0.f, GetWorld()->GetTimeSeconds() - LocalFlightStartTime);
		const float Alpha = FMath::Clamp(ElapsedTime / FMath::Max(EffectiveDuration, KINDA_SMALL_NUMBER), 0.f, 1.f);

		// 직진(FlightDirection)은 그대로 + 좌우 누적 오프셋만 추가
		const FVector Right = FVector::CrossProduct(FVector::UpVector, FlightDirection).GetSafeNormal();
		const FVector NewLocation = StartLocation
			+ FlightDirection * (EffectiveDistance * Alpha)
			+ Right * CurrentSteeringOffset;
		SetActorLocation(NewLocation);

		// 비행 종료 처리는 서버만 (Replicated bool로 클라에 전파)
		if (HasAuthority() && Alpha >= 1.f)
		{
			bIsFlying = false;

			// 끝 지점 도달 → 캐릭터가 자동으로 새에서 내림 (Jump 입력과 동일 효과)
			if (AttachedPlayer)
			{
				AttachedPlayer->Server_ReleaseFromBird();
			}
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
