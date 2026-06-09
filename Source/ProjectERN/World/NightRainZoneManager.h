// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Data/NightRainZoneState.h"
#include "GameFramework/Actor.h"
#include "World/Data/NightRainZonePhaseConfigData.h"
#include "NightRainZoneManager.generated.h"

class UERNSoundSubsystem;
class AERNPlayerController;
class ANightRainZoneCenterPoint;
class UNiagaraComponent;
class UNightRainZoneVisualComponent;
// 자기장 밤의비 상태가 변할 때 호출되는 델리게이트
DECLARE_MULTICAST_DELEGATE_OneParam(FOnNightRainZoneStateChanged, const FNightRainZoneState&);
// 자기장 수렴 후 호출되는 델리게이트
DECLARE_MULTICAST_DELEGATE_OneParam(FOnNightRainZoneShrinkFinished, int32);

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
	FOnNightRainZoneShrinkFinished OnZoneShrinkFinished;
	
	// 자기장 진행 일시정지
	void PauseZoneProgress_ServerOnly();
	// 자기장 진행 재개
	void ResumeZoneProgress_ServerOnly();
	
	bool bIsPauseZone() const { return bZoneProgressPaused; }
	
	// 자기장 면역 상태 부여
	void SetIgnoreNightRainZone(AERNPlayerController* PlayerController, bool bIgnore);
	
	// 안전 지대 좌표 반환
	FVector FindNearestNightLordGraceSafeLocation(const APawn* Pawn) const;
	
protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	void InitializeZone_ServerOnly();
	
	// 클라이언트가 새 ZoneState를 받았을 때 알림
	UFUNCTION()
	void OnRep_ZoneState();
	
	// 서버/클라이언트 로컬 구독자에게 상태 변경 알림
	void BroadcastZoneStateChanged();
	
	// 자기장 수렴 종료 알림
	void BroadcastZoneShrinkFinished();
	
	// Server 전용
	// 페이즈 타이머 설정
	void StartPhase(const FNightRainZonePhaseConfig& PhaseConfig, const FVector& TargetCenterLocation);
	void StopZone();
	
	// 축소 완료 후 최종 위치/반지름으로 상태 고정
	void HandlePhaseFinished();
	
	void StartDamageTimer();
	void StopDamageTimer();
	
	// 서버에서 플레이어 순회하며 처리
	void TickZoneDamage();
	
	// 자기장 안/밖 상태 판정용
	void StartInCircleCheckTimer();
	void StopInCircleCheckTimer();
	void TickInCircleCheck();
	
	// 평면 거리 자기방 안/밖 여부 계산
	bool IsOutsideZone2D(const FVector& WorldLocation, const FVector& Center, float Radius)const;
	
	// 데미지 적용
	void ApplyZoneDamage(APawn* Pawn);
	
	// GameState의 동기화된 서버 시간 반환
	double GetSyncedServerTimeSeconds() const;
	
	// 서버에서 ZoneState 변경하고 알림
	void SetZoneState_ServerOnly(const FNightRainZoneState& NewState);
	
	// 자기장 중심 포인트 수집
	void CollectZoneCenter();
	
	// 자기장 중심 후보 선별
	ANightRainZoneCenterPoint* ChooseNextZoneCenter(int32 CurrentPhaseIndex);
	
	// 초기 자기장 상태 설정
	void StartInitialZoneState();
	
	// 대기상태 종료 핸들
	void HandleWaitFinished();
	
	// 다음 자기장 페이즈 시작
	void StartNextShrinkPhase();
	
	// 수렴 후 대기
	void FreezeCurrentZoneState();
	
	// 다음 자기장 단계가 있는지 확인
	bool HasNextShrinkPhase() const;
	
	// 수렴한 원에 따라 갈 수 있는 자기장 후보 재설정
	void UpdateZoneCenterPoints();
	
	// 자기장 음악 변경
	void ApplyZoneBGM();
	
private:
	// 자기장 설정 Data Asset
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Night Rain Zone", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UNightRainZonePhaseConfigData> ZoneConfig;
	
	UPROPERTY(ReplicatedUsing = OnRep_ZoneState)
	FNightRainZoneState ZoneState;
	
	FTimerHandle DamageTimerHandle;
	FTimerHandle PhaseTimerHandle;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Night Rain Zone|Visual", meta=(AllowPrivateAccess="true"))
	TObjectPtr<USceneComponent> SceneRootComponent;
	
	// 자기장 나이아가라 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Night Rain Zone|Visual", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UNiagaraComponent> NiagaraComponent;
	
	// NightRainZoneManager 에 설정된 자기장 나이아가라 컴포넌트를 시각화 하는 기능을 담당하는 비쥬얼 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Night Rain Zone|Visual", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UNightRainZoneVisualComponent> VisualComponent;
	
	// 자기장 중심 포인트 후보
	UPROPERTY()
	TArray<ANightRainZoneCenterPoint*> CachedZoneCenterPoints;
	
	// 현재 사용중인 자기장 중심 포인트
	UPROPERTY()
	TObjectPtr<ANightRainZoneCenterPoint> CurrentCenterPoint;
	
	FTimerHandle InCircleCheckTimerHandle;
	
	// 자기장 안/밖 여부 확인 주기
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Night Rain Zone|Effect", meta = (AllowPrivateAccess = "true", ClampMin = "0.01"))
	float InCircleCheckInterval = 0.2f;
	
	// 자기장 진행 일시정지 여부
	bool bZoneProgressPaused = false;
	// 수렴 중 일시정지인지 여부
	bool bPausedDuringShrink = false;
	// 일시정지 한 시점의 수렴 진행 시간
	float PausedPhaseRemainingTime = 0.0f;
	
	// 자기장 배경음악
	UPROPERTY()
	UERNSoundSubsystem* SoundSubsystem;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess="true"))
	TArray<TObjectPtr<USoundBase>> ShrinkBGMArray;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(AllowPrivateAccess="true"))
	TArray<TObjectPtr<USoundBase>> RainZonePhaseBGMArray;
};
