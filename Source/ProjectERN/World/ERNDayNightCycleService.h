// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Tickable.h"
#include "ERNDayNightCycleService.generated.h"

class UERNDayNightCycleConfigDataAsset;
class AExponentialHeightFog;
class ASkyLight;
class ADirectionalLight;
class AERNGameState;
class APostProcessVolume;

struct FERNDayNightCycleState;
/**
 * ERNGameState에서 서버의 상태를 가져오고
 * 각자 로컬 월드에서 조명을 어떻게 보이게 만들지 계산해서 처리하는 서비스 시스템
 * ERNWorldManagerSubsystem이 이 서비스를 소유 및 관리
 */
UCLASS()
class PROJECTERN_API UERNDayNightCycleService : public UObject, public FTickableGameObject
{
	GENERATED_BODY()
	
public:
	UERNDayNightCycleService(const FObjectInitializer& ObjectInitializer);
	
	void Initialize(UWorld* InWorld, UERNDayNightCycleConfigDataAsset* InConfig);
	void Shutdown();
	
	virtual void BeginDestroy() override;
	
	//낮밤 전환이 진행 중일 때 로컬 경과 시간을 증가시키고, 진행률 Alpha에 따라 조명을 갱신
	virtual void Tick(float DeltaTime) override;
	virtual bool IsTickable() const override;
	virtual UWorld* GetTickableGameObjectWorld() const override;
	virtual TStatId GetStatId() const override;
	
private:
	//GameState 찾고 상태 변경 델리게이트 바인딩
	void BindGameState();
	void UnbindGameState();
	void TryBindGameStateAndSync();
	
	
	//GameState의 낮밤 상태가 변경되었을 때 호출되는 콜백
	//받은 상태를 SyncFromState()로 넘겨 서비스 상태에 반영
	void HandleDayNightCycleStateChanged(const FERNDayNightCycleState& State);
	
	// 서버 시간 읽기
	void SyncFromState(const FERNDayNightCycleState& State);
	
	void CacheLightingActors();
	//낮밤 진행률 Alpha에 따라 실제 조명을 변경
	void ApplyLighting(float Alpha);
	
	float EvaluateFloatCurve(const UCurveFloat* Curve, float Alpha, float DefaultValue) const;
	FLinearColor EvaluateColorCurve(const UCurveLinearColor* Curve, float Alpha, const FLinearColor& DefaultValue) const;
	
private:
	UPROPERTY()
	TObjectPtr<UERNDayNightCycleConfigDataAsset> Config;
	
	TWeakObjectPtr<UWorld> World;
	TWeakObjectPtr<AERNGameState> BoundGameState;
	
	TWeakObjectPtr<ADirectionalLight> SunLight;
	TWeakObjectPtr<ASkyLight> SkyLight;
	TWeakObjectPtr<AExponentialHeightFog> HeightFog;
	TWeakObjectPtr<APostProcessVolume> PostProcessVolume;
	
	float LocalElapsedSeconds = 0.0f;
	float Duration = 0.0f;
	float SkyLightRecaptureElapsed = 0.0f;
	
	int32 AppliedRevision = INDEX_NONE;
	
	bool bInitialized = false;
	bool bRunning = false;
	
	FTimerHandle GameStateBindRetryTimerHandle;
};
