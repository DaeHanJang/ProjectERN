// Fill out your copyright notice in the Description page of Project Settings.


#include "World/ERNDayNightCycleService.h"

#include "Core/ERNGameState.h"
#include "World/Data/ERNDayNightCycleState.h"
#include "World/Data/ERNDayNightCycleConfigDataAsset.h"

#include "Engine/World.h"
#include "EngineUtils.h"

#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/PostProcessVolume.h"
#include "Components/SkyAtmosphereComponent.h"

#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Curves/CurveLinearColor.h"

UERNDayNightCycleService::UERNDayNightCycleService(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, FTickableGameObject(ETickableTickType::Never)
{
}

void UERNDayNightCycleService::Initialize(UWorld* InWorld, UERNDayNightCycleConfigDataAsset* InConfig)
{
	// 월드+설정 저장, 조명 액터 캐싱, GameState 이벤트 바인딩, 현재 상태 동기화
	if (InWorld == nullptr)
	{
		UE_LOG(LogTemp,Error,TEXT("InWorld is nullptr"));
		return;
	}
	if (InConfig == nullptr)
	{
		UE_LOG(LogTemp,Error,TEXT("InConfig is nullptr"));
		return;
	}
	bInitialized = true;
	World = InWorld;
	Config = InConfig;
	
	CacheLightingActors();
	BindGameState();
	
	if (const AERNGameState* GameState = BoundGameState.Get())
	{
		SyncFromState(GameState->GetDayNightCycleState());
	}
}

void UERNDayNightCycleService::Shutdown()
{
	SetTickableTickType(ETickableTickType::Never);

	UnbindGameState();

	bInitialized = false;
	bRunning = false;
	World.Reset();
	Config = nullptr;
	SunLight.Reset();
	SkyLight.Reset();
}

void UERNDayNightCycleService::BeginDestroy()
{
	Shutdown();
	Super::BeginDestroy();
}

void UERNDayNightCycleService::Tick(float DeltaTime)
{
	if (IsTickable() == false)
	{
		return;
	}
	
	// 로컬 시간만 누적
	LocalElapsedSeconds += DeltaTime;
	
	const float Alpha = FMath::Clamp(LocalElapsedSeconds / Duration, 0.0f, 1.0f);
	ApplyLighting(Alpha);
	
	// 밤이 되었다면 낮밤 전환 서비스의 틱 종료
	if (Alpha >= 1.0f)
	{
		bRunning = false;
		SetTickableTickType(ETickableTickType::Never);
	}
	
}

bool UERNDayNightCycleService::IsTickable() const
{
	return bInitialized
	&& bRunning
	&& World.IsValid()
	&& Config != nullptr
	&& Duration > 0.0f
	&& !HasAnyFlags(RF_ClassDefaultObject | RF_BeginDestroyed);
}

UWorld* UERNDayNightCycleService::GetTickableGameObjectWorld() const
{
	return World.Get();
}

TStatId UERNDayNightCycleService::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UERNDayNightCycleService, STATGROUP_Tickables);
}

// 게임 스테이트랑 델리게이트 연동
void UERNDayNightCycleService::BindGameState()
{
	if (World.IsValid() == false)
	{
		return;
	}
	
	AERNGameState* GameState = World->GetGameState<AERNGameState>();
	if (GameState == nullptr || BoundGameState.Get() == GameState)
	{
		return;
	}
	
	UnbindGameState();
	
	BoundGameState = GameState;
	GameState->OnDayNightCycleStateChanged.AddUObject(this, &UERNDayNightCycleService::HandleDayNightCycleStateChanged);
}

void UERNDayNightCycleService::UnbindGameState()
{
	if (AERNGameState* GameState = BoundGameState.Get())
	{
		GameState->OnDayNightCycleStateChanged.RemoveAll(this);
	}

	BoundGameState.Reset();
}

void UERNDayNightCycleService::HandleDayNightCycleStateChanged(const FERNDayNightCycleState& State)
{
	SyncFromState(State);
}

// 서버 시간 읽기
void UERNDayNightCycleService::SyncFromState(const FERNDayNightCycleState& State)
{
	if (bInitialized == false || AppliedRevision == State.Revision)
	{
		// Applied Revision이 현재 서버의 버전과 같은 수치라면 이미 처리한 내용이므로 생략
		return;
	}
	
	// 버전 최신화
	AppliedRevision = State.Revision;
	
	// 전환 끝나고 틱 꺼야하는지 판단
	if (State.bRunning == false || State.Duration <= 0.0f)
	{
		bRunning = false;
		SetTickableTickType(ETickableTickType::Never);
		return;
	}
	
	const AERNGameState* GameState = BoundGameState.Get();
	if (GameState == nullptr)
	{
		return;
	}
	
	// 서버 시간 읽기
	const double ServerNow = GameState->GetServerWorldTimeSeconds();
	const double Elapsed = ServerNow - State.StartServerWorldTimeSeconds;
	
	// 클라 시간 설정
	Duration = State.Duration;
	LocalElapsedSeconds = FMath::Clamp(static_cast<float>(Elapsed), 0.0f, Duration);
	bRunning = true;
	
	// 설정된 시간에 따른 처리
	ApplyLighting(LocalElapsedSeconds / Duration);
	SetTickableTickType(ETickableTickType::Conditional);
}

void UERNDayNightCycleService::CacheLightingActors()
{
	if (World.IsValid() == false)
	{
		UE_LOG(LogTemp,Warning,TEXT("CacheLightingActors() World is not valid"));
		return;
	}
	
	if (Config == nullptr)
	{
		UE_LOG(LogTemp,Warning,TEXT("CacheLightingActors() Config is not valid"));
		return;
	}
	
	// 필요한 (조명) 엑터들 Config에 설정해둔 태그로 검색하여 캐시화
	for (TActorIterator<ADirectionalLight> It(World.Get()); It; ++It)
	{
		if (It->ActorHasTag(Config->SunLightTag))
		{
			SunLight = *It;
			break;
		}
	}
	
	for (TActorIterator<ASkyLight>It(World.Get()); It; ++It)
	{
		if (It->ActorHasTag(Config->SkyLightTag))
		{
			SkyLight = *It;
			break;
		}
	}
}

void UERNDayNightCycleService::ApplyLighting(float Alpha)
{
	// 태양광 조절
	if (SunLight.IsValid() == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("SunLight is Not Valid"));
		return;
	}

	SunLight->SetActorRotation(FMath::Lerp(Config->DaySunRotation, Config->NightSunRotation, Alpha));

	if (UDirectionalLightComponent* SunComponent = Cast<UDirectionalLightComponent>(SunLight->GetLightComponent()))
	{
		SunComponent->SetIntensity(EvaluateFloatCurve(Config->SunIntensityCurve, Alpha, SunComponent->Intensity));
		SunComponent->SetLightColor(EvaluateColorCurve(Config->SunColorCurve, Alpha, SunComponent->GetLightColor()));
	}
	
	// 스카이 라이트 조절
	if (SkyLight.IsValid() == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("SkyLight is Not Valid"));
		return;
	}
	
	USkyLightComponent* SkyComponent = Cast<USkyLightComponent>(SkyLight->GetLightComponent());
	if (SkyComponent == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("SkyComponent is Not Valid"));
		return;
	}

	SkyComponent->SetIntensity(EvaluateFloatCurve(Config->SkyLightIntensityCurve, Alpha, SkyComponent->Intensity));
	
	if (Config->bRecaptureSkyLight)
	{
		SkyLightRecaptureElapsed += World->GetDeltaSeconds();
		if (SkyLightRecaptureElapsed >= Config->SkyLightRecaptureInterval)
		{
			SkyLightRecaptureElapsed = 0.0f;
			SkyComponent->RecaptureSky();
		}
	}
}

float UERNDayNightCycleService::EvaluateFloatCurve(const UCurveFloat* Curve, float Alpha, float DefaultValue) const
{
	return Curve ? Curve->GetFloatValue(Alpha) : DefaultValue;
}

FLinearColor UERNDayNightCycleService::EvaluateColorCurve(const UCurveLinearColor* Curve, float Alpha,
	const FLinearColor& DefaultValue) const
{
	return Curve ? Curve->GetLinearColorValue(Alpha) : DefaultValue;
}
