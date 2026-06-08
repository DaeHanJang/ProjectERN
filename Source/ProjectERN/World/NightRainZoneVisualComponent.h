// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/NightRainZoneState.h"
#include "NightRainZoneVisualComponent.generated.h"


class UNiagaraComponent;
class UNiagaraSystem;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PROJECTERN_API UNightRainZoneVisualComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UNightRainZoneVisualComponent();

	void SetZoneState(const FNightRainZoneState& NewState);
	void SetNiagaraComponent(UNiagaraComponent* InNiagaraComponent);
	
protected:
	// Called when the game starts
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	void ApplyActiveState();
	bool UpdateVisual();
	
	double GetSyncedServerTimeSeconds() const;
	
private:
	UPROPERTY()
	TObjectPtr<UNiagaraComponent> ZoneNiagaraComponent;
	
	FNightRainZoneState CachedState;
	bool bHasState = false;
	
	// 자기장 비쥬얼 시작 높이, 실제 높이 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess="true"))
	float ZoneVisualCenterZ = 7000.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess="true"))
	float ZoneVisualHeight = 48000.f;
};
