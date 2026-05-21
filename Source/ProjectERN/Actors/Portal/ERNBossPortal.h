// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ERNBossPortal.generated.h"

class UStaticMeshComponent;
class UBoxComponent;
class UNiagaraComponent;
class AProjectERNCharacter;
class UPrimitiveComponent;

/**
 * 최종 보스맵 이동용 포탈
 * 현재 세션의 모든 플레이어가 동시에 트리거 볼륨에 들어가면 ServerTravel로 이동
 */
UCLASS()
class PROJECTERN_API AERNBossPortal : public AActor
{
	GENERATED_BODY()

public:
	AERNBossPortal();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;

	// 포탈 시각용 메시
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> PortalMesh;

	// 오버랩 감지 볼륨
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UBoxComponent> TriggerVolume;

	// 포탈 비주얼 이펙트 (BP에서 NiagaraSystem 할당)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UNiagaraComponent> PortalVFX;

	// 이동할 보스맵 이름
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Portal")
	FString BossMapName = TEXT("Map_BossArena");

	// 현재 포탈 안에 있는 플레이어 수 (UI 피드백용 — 클라에 리플)
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Portal")
	int32 ReadyCount = 0;

	// 오버랩 이벤트
	UFUNCTION()
	void OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnTriggerEndOverlap(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	// 조건 충족 시 ServerTravel
	void CheckAndTriggerTravel();

	// 모든 클라이언트에 로딩 화면 표시
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_ShowLoadingScreen();

private:
	// 서버 권한에서만 의미 — 중복 추가 방지를 위한 캐릭터 목록
	UPROPERTY()
	TArray<TWeakObjectPtr<AProjectERNCharacter>> OverlappingPlayers;

	// 중복 방지 플래그 (서버 권한)
	bool bTravelTriggered = false;
};
