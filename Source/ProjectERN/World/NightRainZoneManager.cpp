// Fill out your copyright notice in the Description page of Project Settings.


#include "World/NightRainZoneManager.h"

#include "EngineUtils.h"
//#include "IDetailTreeNode.h"
#include "ERNMinimapTargetPoint.h"
#include "NiagaraComponent.h"
#include "NightRainZoneCenterPoint.h"
#include "NightRainZoneVisualComponent.h"
#include "Character/Player/ERNPlayerController.h"
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
	
	// 월드에 배치된 NightRainZoneCenterPoint 들이 준비될때 까지 1틱 대기
	GetWorldTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &ANightRainZoneManager::InitializeZone_ServerOnly));
}

void ANightRainZoneManager::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopDamageTimer();
	StopInCircleCheckTimer();

	// 타이머 정리
	UWorld* World = GetWorld();
	if (World && HasAuthority())
	{
		FTimerManager& TimerManager = World->GetTimerManager();
		TimerManager.ClearTimer(DamageTimerHandle);
		TimerManager.ClearTimer(PhaseTimerHandle);
		TimerManager.ClearTimer(InCircleCheckTimerHandle);
	}
	
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
	StartInCircleCheckTimer();
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

void ANightRainZoneManager::PauseZoneProgress_ServerOnly()
{
	if (HasAuthority() == false || bZoneProgressPaused)
	{
		return;
	}
	
	if (GetWorldTimerManager().IsTimerActive(PhaseTimerHandle) == false)
	{
		return;
	}
	
	const double ServerTime = GetSyncedServerTimeSeconds();
	
	// 자기장 진행 일시정지
	bZoneProgressPaused = true;
	bPausedDuringShrink = ZoneState.bShrinking;
	// 자기장 페이즈 진행도 저장
	PausedPhaseRemainingTime = GetWorldTimerManager().GetTimerRemaining(PhaseTimerHandle);
	
	// 페이즈 타이머 제거
	GetWorldTimerManager().ClearTimer(PhaseTimerHandle);
	
	// 페이즈 최신화 (일시정지 상태로 최신화)
	FNightRainZoneState NewState = ZoneState;
	NewState.bProgressPaused = true;
	NewState.PausedAlpha = ZoneState.GetAlpha(ServerTime);
	NewState.Revision++;
	
	SetZoneState_ServerOnly(NewState);
}

void ANightRainZoneManager::ResumeZoneProgress_ServerOnly()
{
	if (HasAuthority() == false || bZoneProgressPaused == false)
	{
		return;
	}
	
	// 멈춘 시점의 시간이 반환됨 (아직 bProgressPaused = true인 상태이기 때문)
	const double ServerTime = GetSyncedServerTimeSeconds();
	
	// 일시정지 하기 전의 남은 진행 시간을 그대로 이어받음
	const float ResumeDuration = FMath::Max(PausedPhaseRemainingTime, 0.01f);
	
	// 자기장 자체의 상태 최신화
	FNightRainZoneState NewState = ZoneState;
	NewState.bProgressPaused = false;
	NewState.PausedAlpha = 0.f;
	NewState.PhaseStartServerWorldTimeSeconds = ServerTime;		// 멈춘 시점의 시간을 가져옴
	NewState.Revision++;
	
	// 수축중이였다면 시간에 맞춰서 중심점, 반지름 정확하게 다시 제공, 수축 타이머 새롭게 재가동
	if (bPausedDuringShrink)
	{
		NewState.bShrinking = true;
		NewState.StartCenter = ZoneState.GetCenterAtTime(ServerTime);
		NewState.StartRadius = ZoneState.GetRadiusAtTime(ServerTime);
		NewState.PhaseDuration = ResumeDuration;
		
		SetZoneState_ServerOnly(NewState);
		
		GetWorldTimerManager().SetTimer(PhaseTimerHandle,this,&ANightRainZoneManager::HandlePhaseFinished,ResumeDuration,false);
	}
	// 수축중이 아니라면 기존에 기다린 시간 반영. 대기 타이머 새롭게 재가동
	else
	{
		NewState.bShrinking = false;
		NewState.PhaseDuration = 0.f;
		NewState.FreezingDuration = ResumeDuration;
		
		SetZoneState_ServerOnly(NewState);
		
		GetWorldTimerManager().SetTimer(PhaseTimerHandle, this, &ANightRainZoneManager::HandlePhaseFinished,ResumeDuration,false);
	}
	
	// 매니저 상태 최신화
	bZoneProgressPaused = false;
	bPausedDuringShrink = false;
	PausedPhaseRemainingTime = 0.f;
}

void ANightRainZoneManager::SetIgnoreNightRainZone(AERNPlayerController* PlayerController, bool bIgnore)
{
	if (PlayerController ==	nullptr)
	{
		return;
	}
}

FVector ANightRainZoneManager::FindNearestNightLordGraceSafeLocation(const APawn* Pawn) const
{
	const FVector CurrentCenter = GetCurrentCenter();
	const float CurrentRadius = GetCurrentRadius();
	
	if (Pawn == nullptr)
	{
		return CurrentCenter;
	}
	
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return CurrentCenter;
	}
	
	const FVector PawnLocation = Pawn->GetActorLocation();
	
	bool bFoundSafeLocation = false;
	FVector BestLocation = CurrentCenter;
	float BestDist = MAX_FLT;
	
	// 미니맵에 밝혀진 화툿불 위치를 대상으로 후보 탐색
	for (TActorIterator<AERNMinimapTargetPoint> It(World); It; ++It)
	{
		AERNMinimapTargetPoint* TargetPoint = *It;
		if (IsValid(TargetPoint) == false)
		{
			continue;
		}
		
		if (TargetPoint->IconType != EERNMinimapIconType::NightLordGrace)
		{
			continue;
		}
		
		const FVector TargetLocation = TargetPoint->GetActorLocation();
		
		// 자기장 범위 밖은 제외
		if (IsOutsideZone2D(TargetLocation, CurrentCenter, CurrentRadius))
		{
			continue;
		}
		
		const float Dist = FVector::DistSquared2D(PawnLocation, TargetLocation);
		if (Dist < BestDist)
		{
			BestDist = Dist;
			BestLocation = TargetLocation;
			bFoundSafeLocation = true;
		}
	}
	
	return bFoundSafeLocation ? BestLocation : CurrentCenter;
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

void ANightRainZoneManager::BroadcastZoneShrinkFinished()
{
	OnZoneShrinkFinished.Broadcast(ZoneState.PhaseIndex);
}

void ANightRainZoneManager::StartPhase(const FNightRainZonePhaseConfig& PhaseConfig, const FVector& TargetCenterLocation)
{
	if (HasAuthority() == false)
	{
		return;
	}
	
	FNightRainZoneState NewState;
	NewState.bShrinking = true;
	NewState.StartCenter = GetCurrentCenter();
	NewState.TargetCenter = TargetCenterLocation;
	NewState.StartRadius = GetCurrentRadius();
	NewState.TargetRadius = PhaseConfig.TargetRadius;
	
	NewState.PhaseDuration = FMath::Max(PhaseConfig.ShrinkDuration, 0.01f);
	NewState.FreezingDuration = PhaseConfig.FreezingDuration;
	NewState.PhaseStartServerWorldTimeSeconds = GetSyncedServerTimeSeconds();
	NewState.PhaseIndex = ZoneState.PhaseIndex + 1;
	NewState.Revision = ZoneState.Revision + 1;
	
	SetZoneState_ServerOnly(NewState);
	
	StartDamageTimer();
	
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
	
	// 자기장 수렴 완료 이벤트 전파
	BroadcastZoneShrinkFinished();
	
	// 현재 수렴 포인트가 처리할 이벤트가 있다면 실행
	CurrentCenterPoint->HandleZoneShrinkFinished();
	
	// 자기장 수렴 후 대기
	FreezeCurrentZoneState();
	
	// 자기장 후보 재설정
	UpdateZoneCenterPoints();
	
	if (HasNextShrinkPhase())
	{
		UE_LOG(LogTemp, Warning, TEXT("다음 장소 있음"));
		GetWorldTimerManager().SetTimer(PhaseTimerHandle, this, &ANightRainZoneManager::HandleWaitFinished, ZoneState.FreezingDuration, false);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("다음 장소 없음"));
	}
}

void ANightRainZoneManager::StartDamageTimer()
{
	if (HasAuthority() == false)
	{
		return;
	}
	
	if (ZoneConfig == nullptr)
	{
		return;
	}
	
	if (ZoneConfig->PhaseConfigs.IsValidIndex(ZoneState.PhaseIndex) == false)
	{
		return;
	}
	
	GetWorldTimerManager().ClearTimer(DamageTimerHandle);
	
	const float Interval = FMath::Max(  ZoneConfig->PhaseConfigs[ZoneState.PhaseIndex].DamageTickInterval, 0.1f);
	
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
		AERNPlayerController* PC = Cast<AERNPlayerController>(It->Get());
		if (PC == nullptr)
		{
			continue;
		}
		
		if (PC->bIsIgnoreNightRainZone())
		{
			continue;;
		}
		
		APawn* Pawn = PC->GetPawn();
		if (Pawn == nullptr)
		{
			continue;
		}
		
		// 데미지 적용
		if (IsOutsideZone2D(Pawn->GetActorLocation(), Center, Radius))
		{
			ApplyZoneDamage(Pawn);
		}
	}
}

void ANightRainZoneManager::StartInCircleCheckTimer()
{
	if (HasAuthority() == false)
	{
		return;
	}
	
	TickInCircleCheck();
	
	GetWorldTimerManager().SetTimer(InCircleCheckTimerHandle, this, &ANightRainZoneManager::TickInCircleCheck, InCircleCheckInterval, true);
}

void ANightRainZoneManager::StopInCircleCheckTimer()
{
	if (HasAuthority() == false)
	{
		return;
	}
	
	GetWorldTimerManager().ClearTimer(InCircleCheckTimerHandle);
	
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		AERNPlayerController* PC = Cast<AERNPlayerController>(It->Get());
		
		if(PC == nullptr)
		{
			continue;
		}

		PC->UpdateNightRainPostProcessState_ServerOnly(false);
	}
}

void ANightRainZoneManager::TickInCircleCheck()
{
	if (HasAuthority() == false)
	{
		return;
	}
	
	const FVector Center = GetCurrentCenter();
	const float Radius = GetCurrentRadius();
	
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		AERNPlayerController* PC = Cast<AERNPlayerController>(It->Get());
		if (PC == nullptr)
		{
			continue;
		}
		
		
		if (PC->bIsIgnoreNightRainZone())
		{
			continue;
		}
		
		APawn* Pawn = PC->GetPawn();
		if (Pawn == nullptr)
		{
			PC->UpdateNightRainPostProcessState_ServerOnly(false);
			continue;
		}
		
		const bool bOutsideCircle = IsOutsideZone2D(Pawn->GetActorLocation(), Center, Radius);
		PC->UpdateNightRainPostProcessState_ServerOnly(bOutsideCircle);
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
	
	if (ZoneConfig == nullptr)
	{
		return;
	}
	
	
	if (ZoneConfig->PhaseConfigs.IsValidIndex(ZoneState.PhaseIndex) == false)
	{
		return;
	}
	
	UGameplayStatics::ApplyDamage(Pawn, ZoneConfig->PhaseConfigs[ZoneState.PhaseIndex].DamagePerTick, nullptr, this, UDamageType::StaticClass());
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
	if (ZoneConfig == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("NightRainZone: ZoneConfig is null."));
		return;
	}
	
	if (ZoneConfig->PhaseConfigs.IsValidIndex(0) == false)
	{
		return;
	}

	const FNightRainZonePhaseConfig& InitPhaseConfig = ZoneConfig->PhaseConfigs[0];

	
	FNightRainZoneState NewState;
	NewState.bShrinking = false;

	NewState.StartCenter = InitPhaseConfig.TargetCenter;
	NewState.TargetCenter = InitPhaseConfig.TargetCenter;

	NewState.StartRadius = InitPhaseConfig.TargetRadius;
	NewState.TargetRadius = InitPhaseConfig.TargetRadius;

	NewState.PhaseDuration = 0.f;
	NewState.FreezingDuration = InitPhaseConfig.FreezingDuration;
	NewState.PhaseStartServerWorldTimeSeconds = GetSyncedServerTimeSeconds();
	NewState.PhaseIndex = 0;
	NewState.Revision = ZoneState.Revision + 1;

	SetZoneState_ServerOnly(NewState);

	StartDamageTimer();
	
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
	if (HasAuthority() == false)
	{
		return;
	}
	
	if (HasNextShrinkPhase() == false)
	{
		return;
	}
	
	if (ZoneConfig == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("NightRainZone: ZoneConfig is null."));
		return;
	}

	const int32 NextPhaseIndex = ZoneState.PhaseIndex + 1;
	
	if (ZoneConfig->PhaseConfigs.IsValidIndex(NextPhaseIndex) == false)
	{
		return;
	}

	CurrentCenterPoint = ChooseNextZoneCenter(ZoneState.PhaseIndex);
	if (CurrentCenterPoint == nullptr)
	{
		return;
	}

	const FNightRainZonePhaseConfig& NextPhaseConfig = ZoneConfig->PhaseConfigs[NextPhaseIndex];

	StartPhase(NextPhaseConfig, CurrentCenterPoint->GetActorLocation());
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
	if (ZoneConfig == nullptr)
	{
		return false;
	}
	const int32 NextPhaseIndex = ZoneState.PhaseIndex + 1;

	if (ZoneConfig->PhaseConfigs.IsValidIndex(NextPhaseIndex) == false)
	{
		return false;
	}
	
	// 아직 남아있는 자기장 단계가 있다면 도달할 수 있는지 확인
	for (ANightRainZoneCenterPoint* CenterPoint : CachedZoneCenterPoints)
	{
		if (IsValid(CenterPoint) == false)
		{
			continue;
		}
	
		// 활성화 가능하고 다음 단계인 페이즈가 있다면 추가 진행 가능
		if (CenterPoint->bEnabled && (CenterPoint->ZoneLevel == ZoneState.PhaseIndex + 1))
		{
			return true;
		}
	}
	
	return false;
}

void ANightRainZoneManager::UpdateZoneCenterPoints()
{
	FVector CurrentZoneLocation = GetCurrentCenter();
	float CurrentRadius = GetCurrentRadius();
	for (ANightRainZoneCenterPoint* CenterPoint : CachedZoneCenterPoints)
	{
		if (IsValid(CenterPoint) == false)
		{
			continue;
		}
	
		if (CenterPoint->bEnabled == false)
		{
			continue;
		}
		
		FVector CenterPointLocation = CenterPoint->GetActorLocation();
		CenterPointLocation.Z = 0;
		
		if ( FVector::DistSquared2D(CurrentZoneLocation, CenterPointLocation ) > FMath::Square(CurrentRadius))
		{
			CenterPoint->bEnabled = false;
		}
	}
}
