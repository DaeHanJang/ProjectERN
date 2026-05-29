// Fill out your copyright notice in the Description page of Project Settings.


#include "Actors/AoE/ERNAoEActor.h"
#include "Components/SceneComponent.h"
#include "DrawDebugHelpers.h"
#include "NiagaraComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "TimerManager.h"

AERNAoEActor::AERNAoEActor()
{
	PrimaryActorTick.bCanEverTick = false;
	
	SetReplicates(true);
	SetReplicateMovement(false);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);
	
	AoENiagaraComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("AoENiagaraComponent"));
	AoENiagaraComponent->SetupAttachment(SceneRoot);
	AoENiagaraComponent->SetAutoActivate(false);
}

void AERNAoEActor::InitializeAoE(AActor* InSourceActor)
{
	SourceActor = InSourceActor;
}

void AERNAoEActor::BeginPlay()
{
	Super::BeginPlay();
	
	// 값 세팅이 잘 되어 있을 때만 작동
	if (!IsAoEConfigured())
	{
		if (HasAuthority())
		{
			Destroy();
		}
		return;
	}
	
	// 나이아가라 적용
	if (AoENiagaraSystem)
	{
		AoENiagaraComponent->SetAsset(AoENiagaraSystem);
		AoENiagaraComponent->Activate(true);
	}
	
	// 서버에서만 실행
	if (!HasAuthority())
	{
		return;
	}
	
	// 지속 시간 적용
	SetLifeSpan(Duration);
	// 타이머 시작
	StartAoETick();
}

void AERNAoEActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 타이머 종료
	StopAoETick();

	Super::EndPlay(EndPlayReason);
}

bool AERNAoEActor::IsValidAoETarget(AActor* TargetActor, UPrimitiveComponent* OverlappedComponent) const
{
	// ProjectERNCharacter같은 경우 특정 컴포넌트만 적용하도록 처리
	return TargetActor && TargetActor != this && OverlappedComponent;
}

void AERNAoEActor::ApplyAoEToTarget(AActor* TargetActor)
{
	// 적용 대상 설정
	// 파생 클래스에서 override해서 사용
}

bool AERNAoEActor::IsAoEConfigured() const
{
	return Radius > 0.f
		&& Duration > 0.f
		&& TickInterval > 0.f;
}

void AERNAoEActor::StopAoETick()
{
	GetWorldTimerManager().ClearTimer(AoETickTimerHandle);
}

void AERNAoEActor::StartAoETick()
{
	// 첫 틱 적용 여부
	if (bApplyFirstTickImmediately)
	{
		ApplyAoE();
	}

	// 타이머 시작
	GetWorldTimerManager().SetTimer(
		AoETickTimerHandle,
		this,
		&AERNAoEActor::ApplyAoE,
		TickInterval,
		true);
}

void AERNAoEActor::ApplyAoE()
{
	UWorld* World = GetWorld();
	if (!World || Radius <= 0.f)
	{
		return;
	}
	
	// Origin 설정
	const FVector Origin = GetActorLocation();

	// 디버그 적용
	if (bDrawDebug)
	{
		DrawDebugSphere(
			World,
			Origin,
			Radius,
			DebugSegments,
			DebugColor,
			false,
			TickInterval,
			0,
			DebugThickness);
	}

	// Overlap 결과를 담을 배열
	TArray<FOverlapResult> OverlapResults;

	// ECC_Pawn채널만 적용
	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ERNAoEOverlap), false);
	// 자기 자신 제외
	QueryParams.AddIgnoredActor(this);

	// 실제 검색 적용 로직
	const bool bHasOverlap = World->OverlapMultiByObjectType(
		OverlapResults,
		Origin,
		FQuat::Identity,
		ObjectQueryParams,
		FCollisionShape::MakeSphere(Radius),
		QueryParams);

	// Overlap 대상이 없으면 종료
	if (!bHasOverlap)
	{
		return;
	}

	// 적용 Actor (중복 방지를 위해 TSet)
	TSet<AActor*> AppliedActors;

	for (const FOverlapResult& OverlapResult : OverlapResults)
	{
		AActor* TargetActor = OverlapResult.GetActor();
		// 특정 컴포넌트에만 적용하기 위해 추가
		UPrimitiveComponent* OverlappedComponent = OverlapResult.GetComponent();
		// 대상 없거나 이미 적용했으면 스킵
		if (!TargetActor || AppliedActors.Contains(TargetActor))
		{
			continue;
		}

		// 유효한 대상이 아니면 스킵
		if (!IsValidAoETarget(TargetActor, OverlappedComponent))
		{
			continue;
		}

		AppliedActors.Add(TargetActor);
		ApplyAoEToTarget(TargetActor);
	}
}
