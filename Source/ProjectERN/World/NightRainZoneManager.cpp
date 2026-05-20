// Fill out your copyright notice in the Description page of Project Settings.


#include "World/NightRainZoneManager.h"

#include "EngineUtils.h"
#include "NiagaraComponent.h"
#include "NightRainZoneCenterPoint.h"
#include "NightRainZoneVisualComponent.h"
#include "GameFramework/GameStateBase.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"

// Sets default values
ANightRainZoneManager::ANightRainZoneManager()
{
	bReplicates = true;
	bAlwaysRelevant = true;
	PrimaryActorTick.bCanEverTick = false;
	SetCanBeDamaged(false);
	
	SceneRootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRootComponent"));
	SetRootComponent(SceneRootComponent);

	NiagaraComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("ZoneNiagaraComponent"));
	NiagaraComponent->SetupAttachment(SceneRootComponent);
	NiagaraComponent->SetAutoActivate(false);
	
	VisualComponent = CreateDefaultSubobject<UNightRainZoneVisualComponent>(TEXT("VisualComponent"));
}

// Called when the game starts or when spawned
void ANightRainZoneManager::BeginPlay()
{
	Super::BeginPlay();
	
	if (VisualComponent)
	{
		VisualComponent->SetNiagaraComponent(NiagaraComponent);
	}
	
	if (HasAuthority() == false)
	{
		return;
	}
	
	GetWorldTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &ANightRainZoneManager::InitializeZone_ServerOnly));
}

void ANightRainZoneManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopDamageTimer();
	GetWorldTimerManager().ClearTimer(PhaseTimerHandle);
	
	Super::EndPlay(EndPlayReason);
}

void ANightRainZoneManager::InitializeZone_ServerOnly()
{
	if (HasAuthority() == false)
	{
		return;
	}

	CollectZoneCenter();

	StartInitialZoneState();
	StartDamageTimer();
}

void ANightRainZoneManager::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(ANightRainZoneManager, ZoneState);
}

FVector ANightRainZoneManager::GetCurrentCenter() const
{
	return ZoneState.GetCenterAtTime(GetSyncedServerTimeSeconds());
}

float ANightRainZoneManager::GetCurrentRadius() const
{
	return ZoneState.GetRadiusAtTime(GetSyncedServerTimeSeconds());
}

void ANightRainZoneManager::OnRep_ZoneState()
{
	BroadcastZoneStateChanged();
	
	if (VisualComponent)
	{
		VisualComponent->SetZoneState(ZoneState);
	}
}

void ANightRainZoneManager::BroadcastZoneStateChanged()
{
	OnZoneStateChanged.Broadcast(ZoneState);
}

void ANightRainZoneManager::StartInitialPhase()
{
	StartPhase(InitPhaseConfig);
}

void ANightRainZoneManager::StartPhase(const FNightRainZonePhaseConfig& PhaseConfig)
{
	if (HasAuthority() == false)
	{
		return;
	}
	
	FNightRainZoneState NewState;
	NewState.bShrinking = true;
	NewState.StartCenter = PhaseConfig.StartCenter;
	NewState.TargetCenter = PhaseConfig.TargetCenter;
	NewState.StartRadius = PhaseConfig.StartRadius;
	NewState.TargetRadius = PhaseConfig.TargetRadius;
	NewState.PhaseDuration = FMath::Max(PhaseConfig.Duration, 0.01f);
	NewState.FreezingDuration = PhaseConfig.FreezingDuration;
	NewState.PhaseStartServerWorldTimeSeconds = GetSyncedServerTimeSeconds();
	NewState.PhaseIndex = ZoneState.PhaseIndex + 1;
	NewState.Revision = ZoneState.Revision + 1;
	
	SetZoneState_ServerOnly(NewState);
	
	GetWorldTimerManager().SetTimer(PhaseTimerHandle, this, &ANightRainZoneManager::HandlePhaseFinished, NewState.PhaseDuration, false);
}

void ANightRainZoneManager::StopZone()
{
	if (HasAuthority() == false)
	{
		return;
	}
	
	GetWorldTimerManager().ClearTimer(PhaseTimerHandle);
	
	FNightRainZoneState NewState = ZoneState;
	NewState.bShrinking = false;
	NewState.Revision++;
	
	SetZoneState_ServerOnly(NewState);
}

void ANightRainZoneManager::HandlePhaseFinished()
{
	if (HasAuthority() == false || ZoneState.bShrinking == false)
	{
		return;
	}
	
	// 자기장 수렴 대기
	FreezeCurrentZoneState();
	
	if (HasNextShrinkPhase())
	{
		GetWorldTimerManager().SetTimer(PhaseTimerHandle, this, &ANightRainZoneManager::HandleWaitFinished, ZoneState.FreezingDuration, false);
	}
}

void ANightRainZoneManager::StartDamageTimer()
{
	if (HasAuthority() == false)
	{
		return;
	}
	
	const float Interval = FMath::Max(InitPhaseConfig.DamageTickInterval, 0.1f);
	
	GetWorldTimerManager().SetTimer(DamageTimerHandle, this, &ANightRainZoneManager::TickZoneDamage, Interval, true);
}

void ANightRainZoneManager::StopDamageTimer()
{
	if (HasAuthority() == false)
	{
		return;
	}
	
	GetWorldTimerManager().ClearTimer(DamageTimerHandle);
}

void ANightRainZoneManager::TickZoneDamage()
{
	if (HasAuthority() == false)
	{
		return;
	}
	
	const FVector Center = GetCurrentCenter();
	const float Radius = GetCurrentRadius();
	
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		APawn* Pawn = PC ? PC->GetPawn() : nullptr;
		
		if (Pawn == nullptr)
		{
			continue;
		}

		const float Distance2D = FVector2D::Distance(FVector2D(Pawn->GetActorLocation().X, Pawn->GetActorLocation().Y),FVector2D(Center.X, Center.Y));

		UE_LOG(LogTemp,Warning,TEXT("NightRain DamageCheck Pawn=%s Center=%s Dist2D=%.1f Radius=%.1f Outside=%s Phase =%d"),
			*Pawn->GetActorLocation().ToString(),
			*Center.ToString(),
			Distance2D,
			Radius,
			IsOutsideZone2D(Pawn->GetActorLocation(), Center, Radius) ? TEXT("true") : TEXT("false"),
			ZoneState.PhaseIndex);
		
		//자기장 (밤의비) 밖에 누워있는 플레이어는 몹으로 인식함으로 예외 해야함
		//if (Pawn)
		
		if (IsOutsideZone2D(Pawn->GetActorLocation(), Center, Radius))
		{
			ApplyZoneDamage(Pawn);
		}
	}
}

bool ANightRainZoneManager::IsOutsideZone2D(const FVector& WorldLocation, const FVector& Center, float Radius) const
{
	const FVector2D PlayerXY(WorldLocation.X, WorldLocation.Y);
	const FVector2D CenterXY(Center.X, Center.Y);
	
	return FVector2D::DistSquared(PlayerXY, CenterXY) > FMath::Square(Radius);
}

void ANightRainZoneManager::ApplyZoneDamage(APawn* Pawn)
{
	if (Pawn == nullptr)
	{
		return;
	}
	
	UGameplayStatics::ApplyDamage(Pawn, InitPhaseConfig.DamagePerTick, nullptr, this, UDamageType::StaticClass());
}

double ANightRainZoneManager::GetSyncedServerTimeSeconds() const
{
	if (const UWorld* World = GetWorld())
	{
		const AGameStateBase* GameState = World->GetGameState();
		return GameState ? GameState->GetServerWorldTimeSeconds() : World->GetTimeSeconds();
	}

	return 0.0;
}

void ANightRainZoneManager::SetZoneState_ServerOnly(const FNightRainZoneState& NewState)
{
	if (HasAuthority() == false)
	{
		return;
	}
	
	ZoneState = NewState;
	
	BroadcastZoneStateChanged();
	
	if (VisualComponent)
	{
		VisualComponent->SetZoneState(ZoneState);
	}
}

void ANightRainZoneManager::CollectZoneCenter()
{
	UWorld* World = GetWorld();
	
	if (World == nullptr)
	{
		UE_LOG(LogTemp,Error,TEXT("World is nullptr"));
		return;
	}

	CachedZoneCenterPoints.Reset();
	constexpr int32 MaxExpectedCenterPoints = 64;
	CachedZoneCenterPoints.Reserve(MaxExpectedCenterPoints);
	
	for (TActorIterator<ANightRainZoneCenterPoint> It(World); It; ++It)
	{
		ANightRainZoneCenterPoint* TargetPoint = *It;
		if (TargetPoint == nullptr)
		{
			continue;
		}
		CachedZoneCenterPoints.Add(TargetPoint);
	}
}

ANightRainZoneCenterPoint* ANightRainZoneManager::ChooseNextZoneCenter(int32 CurrentPhaseIndex)
{
	TArray<ANightRainZoneCenterPoint*> Candidates;
	
	// 현재 진행된 자기장 페이즈에 따라 자기장 후보를 선별
	for (ANightRainZoneCenterPoint* TargetPoint : CachedZoneCenterPoints)
	{
		if (TargetPoint == nullptr)
		{
			continue;
		}
		
		if (TargetPoint->bEnabled == false)
		{
			continue;
		}
		
		// 도착할 다음 페이즈에 해당하는 타겟
		if (TargetPoint->ZoneLevel == (CurrentPhaseIndex + 1))
		{
			Candidates.Add(TargetPoint);
		}
	}
	
	if (Candidates.Num() == 0)
	{
		UE_LOG(LogTemp,Warning,TEXT("No candidates found"));
		return nullptr;
	}
	// 후보 중 랜덤 선택

	int32 RandomIndex = FMath::RandRange(0, Candidates.Num() - 1);
	return Candidates[RandomIndex];
	
}

void ANightRainZoneManager::StartInitialZoneState()
{
	FNightRainZoneState NewState;
	NewState.bShrinking = false;

	NewState.StartCenter = InitPhaseConfig.StartCenter;
	NewState.TargetCenter = InitPhaseConfig.StartCenter;

	NewState.StartRadius = InitPhaseConfig.StartRadius;
	NewState.TargetRadius = InitPhaseConfig.StartRadius;

	NewState.PhaseDuration = 0.f;
	NewState.FreezingDuration = InitPhaseConfig.FreezingDuration;
	NewState.PhaseStartServerWorldTimeSeconds = GetSyncedServerTimeSeconds();
	NewState.PhaseIndex = 0;
	NewState.Revision = ZoneState.Revision + 1;

	SetZoneState_ServerOnly(NewState);

	GetWorldTimerManager().SetTimer(
		PhaseTimerHandle,
		this,
		&ANightRainZoneManager::HandleWaitFinished,
		NewState.FreezingDuration,
		false);
}

void ANightRainZoneManager::HandleWaitFinished()
{
	if (HasAuthority() == false)
	{
		return;
	}

	StartNextShrinkPhase();
}

void ANightRainZoneManager::StartNextShrinkPhase()
{
	if (HasNextShrinkPhase() == false)
	{
		return;
	}

	const int32 ConfigIndex = ZoneState.PhaseIndex;
	const int32 NextPhaseIndex = ZoneState.PhaseIndex + 1;
	
	if (ShrinkPhaseConfigs.IsValidIndex(ConfigIndex) == false)
	{
		return;
	}
	
	ANightRainZoneCenterPoint* NextCenterPoint = ChooseNextZoneCenter(ZoneState.PhaseIndex);
	if (NextCenterPoint == nullptr)
	{
		return;
	}
	
	FNightRainZonePhaseConfig NextPhaseConfig = ShrinkPhaseConfigs[ConfigIndex];

	NextPhaseConfig.StartCenter = ZoneState.TargetCenter;
	NextPhaseConfig.TargetCenter = NextCenterPoint->GetActorLocation();

	NextPhaseConfig.StartRadius = ZoneState.TargetRadius;

	StartPhase(NextPhaseConfig);
}

void ANightRainZoneManager::FreezeCurrentZoneState()
{
	FNightRainZoneState NewState = ZoneState;

	NewState.bShrinking = false;
	NewState.StartCenter = ZoneState.TargetCenter;
	NewState.TargetCenter = ZoneState.TargetCenter;
	NewState.StartRadius = ZoneState.TargetRadius;
	NewState.TargetRadius = ZoneState.TargetRadius;
	NewState.PhaseDuration = 0.f;
	NewState.PhaseStartServerWorldTimeSeconds = GetSyncedServerTimeSeconds();
	NewState.Revision++;
	
	SetZoneState_ServerOnly(NewState);
}

bool ANightRainZoneManager::HasNextShrinkPhase() const
{
	return ZoneState.PhaseIndex < ShrinkPhaseConfigs.Num();
}


