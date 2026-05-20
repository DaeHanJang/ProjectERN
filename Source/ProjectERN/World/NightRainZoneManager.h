// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Data/NightRainZoneState.h"
#include "GameFramework/Actor.h"
#include "NightRainZoneManager.generated.h"

class UNiagaraComponent;
class UNightRainZoneVisualComponent;
// 자기장 밤의비 상태가 변할 때 호출되는 델리게이트
DECLARE_MULTICAST_DELEGATE_OneParam(FOnNightRainZoneStateChanged, const FNightRainZoneState&);


//에디터에서 설정할 초기 설정값
USTRUCT(BlueprintType)
struct FNightRainZonePhaseConfig
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FVector StartCenter = FVector::ZeroVector;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FVector TargetCenter = FVector::ZeroVector;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float StartRadius = 100000.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float TargetRadius = 20000.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float Duration = 300.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float DamagePerTick = 10.f;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	float DamageTickInterval = 1.f;
};


// Is Spatially Loaded = false 를 전제로 만들어진 매니저 엑터입니다. 월드 파티션에서 배치할 때, 꼭 Is Spatially Loaded = false 를 설정해주세요.
UCLASS()
class PROJECTERN_API ANightRainZoneManager : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ANightRainZoneManager();
	
	// ZoneState를 복제 목록에 등록
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	const FNightRainZoneState& GetZoneState() const { return ZoneState; }
	
	bool IsZoneRunning() const { return ZoneState.bShrinking; }
	
	// 현재 서버 시간 기준 자기장(밤의비) 중심,반지름 계산
	FVector GetCurrentCenter() const;
	float GetCurrentRadius() const;
	
	FOnNightRainZoneStateChanged OnZoneStateChanged;
	
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	
	// 클라이언트가 새 ZoneState를 받았을 때 알림
	UFUNCTION()
	void OnRep_ZoneState();
	
	// 서버/클라이언트 로컬 구독자에게 상태 변경 알림
	void BroadcastZoneStateChanged();
	
	// Server 전용
	
	// InitPhaseConfig 에 따라 자기장 페이즈 시작
	void StartInitialPhase();
	
	// 페이즈 타이머 설정
	void StartPhase(const FNightRainZonePhaseConfig& PhaseConfig);
	void StopZone();
	
	// 축소 완료 후 최종 위치/반지름으로 상태 고정
	void HandlePhaseFinished();
	
	void StartDamageTimer();
	void StopDamageTimer();
	
	// 서버에서 플레이어 순회하며 처리
	void TickZoneDamage();
	
	// 평면 거리 자기방 안/밖 여부 계산
	bool IsOutsideZone2D(const FVector& WorldLocation, const FVector& Center, float Radius)const;
	
	// 데미지 적용
	void ApplyZoneDamage(APawn* Pawn);
	
	// GameState의 동기화된 서버 시간 반환
	double GetSyncedServerTimeSeconds() const;
	
	// 서버에서 ZoneState 변경하고 알림
	void SetZoneState_ServerOnly(const FNightRainZoneState& NewState);
	
private:
	UPROPERTY(EditAnywhere, Category="Night Rain Zone")
	FNightRainZonePhaseConfig InitPhaseConfig;
	
	UPROPERTY(ReplicatedUsing = OnRep_ZoneState)
	FNightRainZoneState ZoneState;
	
	FTimerHandle DamageTimerHandle;
	FTimerHandle PhaseTimerHandle;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Night Rain Zone|Visual", meta=(AllowPrivateAccess="true"))
	TObjectPtr<USceneComponent> SceneRootComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Night Rain Zone|Visual", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UNiagaraComponent> NiagaraComponent;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Night Rain Zone|Visual", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UNightRainZoneVisualComponent> VisualComponent;
};
