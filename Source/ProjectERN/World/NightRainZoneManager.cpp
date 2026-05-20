// Fill out your copyright notice in the Description page of Project Settings.


#include "World/NightRainZoneManager.h"

#include "NiagaraComponent.h"
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
		VisualComponent->SetZoneState(ZoneState);
	}
	
	if (HasAuthority() == false)
	{
		return;
	}
	
	StartInitialPhase();
	StartDamageTimer();
}

void ANightRainZoneManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopDamageTimer();
	GetWorldTimerManager().ClearTimer(PhaseTimerHandle);
	
	Super::EndPlay(EndPlayReason);
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
	
	FNightRainZoneState NewState = ZoneState;
	NewState.StartCenter = ZoneState.TargetCenter;
	NewState.TargetCenter = ZoneState.TargetCenter;
	NewState.StartRadius = ZoneState.TargetRadius;
	NewState.TargetRadius = ZoneState.TargetRadius;
	NewState.PhaseStartServerWorldTimeSeconds = GetSyncedServerTimeSeconds();
	NewState.PhaseDuration = 0.f;
	NewState.Revision++;
	
	SetZoneState_ServerOnly(NewState);
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

		UE_LOG(LogTemp,Warning,TEXT("NightRain DamageCheck Pawn=%s Center=%s Dist2D=%.1f Radius=%.1f Outside=%s"),
			*Pawn->GetActorLocation().ToString(),
			*Center.ToString(),
			Distance2D,
			Radius,
			IsOutsideZone2D(Pawn->GetActorLocation(), Center, Radius) ? TEXT("true") : TEXT("false"));
		
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


