// Fill out your copyright notice in the Description page of Project Settings.


#include "World/NightRainZoneVisualComponent.h"
#include "NiagaraComponent.h"
#include "GameFramework/GameStateBase.h"


// Sets default values for this component's properties
UNightRainZoneVisualComponent::UNightRainZoneVisualComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	SetComponentTickInterval(0.2f);
}

void UNightRainZoneVisualComponent::SetZoneState(const FNightRainZoneState& NewState)
{
	CachedState = NewState;
	bHasState = true;
	
	ApplyActiveState();
}

void UNightRainZoneVisualComponent::SetNiagaraComponent(UNiagaraComponent* InNiagaraComponent)
{
	ZoneNiagaraComponent = InNiagaraComponent;
	
	if (ZoneNiagaraComponent == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("NightRainZone ZoneNiagara Component is NULL"));
		SetComponentTickEnabled(false);
		return;
	}
	
	ZoneNiagaraComponent->SetAutoActivate(false);
	ZoneNiagaraComponent->DeactivateImmediate();
	
	ApplyActiveState();
}

// Called when the game starts
void UNightRainZoneVisualComponent::BeginPlay()
{
	Super::BeginPlay();

	SetComponentTickInterval(VisualUpdateInterval);
	SetComponentTickEnabled(false);
}

void UNightRainZoneVisualComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

// Called every frame
void UNightRainZoneVisualComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	if (bHasState == false || CachedState.bShrinking == false)
	{
		return;
	}
	
	if (UpdateVisual() == false)
	{
		SetComponentTickEnabled(false);

		if (ZoneNiagaraComponent)
		{
			ZoneNiagaraComponent->DeactivateImmediate();
		}
	}
}

void UNightRainZoneVisualComponent::ApplyActiveState()
{
	if (bHasState == false || ZoneNiagaraComponent == nullptr)
	{
		SetComponentTickEnabled(false);
		
		if (ZoneNiagaraComponent)
		{
			ZoneNiagaraComponent->DeactivateImmediate();
		}
		
		return;
	}
	
	const bool bVisualValid = UpdateVisual();

	if (bVisualValid == false)
	{
		SetComponentTickEnabled(false);
		ZoneNiagaraComponent->DeactivateImmediate();
		return;
	}

	if (ZoneNiagaraComponent->IsActive() == false)
	{
		ZoneNiagaraComponent->Activate(true);
	}

	SetComponentTickEnabled(CachedState.bShrinking);
}

bool UNightRainZoneVisualComponent::UpdateVisual()
{
	if (ZoneNiagaraComponent == nullptr || bHasState == false)
	{
		return false;
	}

	const double ServerTime = GetSyncedServerTimeSeconds();

	const FVector ZoneCenter = CachedState.GetCenterAtTime(ServerTime);
	const float ZoneRadius = CachedState.GetRadiusAtTime(ServerTime);
	

	/*
	const float SafeTextureAspect = FMath::Max(1.f, 0.001f);
	const float DynamicTileWorldSizeU = TileWorldSizeV * SafeTextureAspect;

	ZoneNiagaraComponent->SetVariableVec3(TEXT("User.ZoneCenter"), MaterialZoneCenter);
	ZoneNiagaraComponent->SetVariableFloat(TEXT("User.TileWorldSizeU"), DynamicTileWorldSizeU);
	ZoneNiagaraComponent->SetVariableFloat(TEXT("User.TileWorldSizeV"), TileWorldSizeV);
	*/

	if (ZoneRadius <= KINDA_SMALL_NUMBER)
	{
		return false;
	}

	constexpr float MeshBaseRadius = 100.f;
	constexpr float MeshBaseHeight = 100.f;

	FVector VisualLocation = ZoneCenter;
	VisualLocation.Z = ZoneVisualCenterZ - ZoneVisualHeight * 0.5f;

	const FVector VisualScale(
		ZoneRadius / MeshBaseRadius,
		ZoneRadius / MeshBaseRadius,
		ZoneVisualHeight / MeshBaseHeight
	);
	
	//나이아가라에 넘겨줄 변수
	ZoneNiagaraComponent->SetWorldLocation(VisualLocation);
	ZoneNiagaraComponent->SetWorldScale3D(VisualScale);
	
	ZoneNiagaraComponent->SetVariableFloat(TEXT("User.ZoneBottomZ"), VisualLocation.Z);
	ZoneNiagaraComponent->SetVariableFloat(TEXT("User.ZoneHeight"), ZoneVisualHeight);
	
	return true;
}

double UNightRainZoneVisualComponent::GetSyncedServerTimeSeconds() const
{
	if (const UWorld* World = GetWorld())
	{
		const AGameStateBase* GameState = World->GetGameState();
		return GameState ? GameState->GetServerWorldTimeSeconds() : World->GetTimeSeconds();
	}

	return 0.0;
}




