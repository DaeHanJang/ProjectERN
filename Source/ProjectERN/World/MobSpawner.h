// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MobSpawner.generated.h"

struct FMobRuntimeState;
class UMobSpawnPointComponent;
class AERNEnemyCharacter;

UCLASS()
class PROJECTERN_API AMobSpawner : public AActor
{
	GENERATED_BODY()
	
public:
	// Sets default values for this actor's properties
	AMobSpawner();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	FName GetMobSpawnerID() const { return MobSpawnerID; }
	void SetMobSpawnerID(const FName NewID) { MobSpawnerID = NewID; }
	
	// 근처의 PatrolPoint 수집
	UFUNCTION(BlueprintCallable, CallInEditor, Category="Patrol")
	void CollectPatrolPoints();
	
	UFUNCTION(BlueprintCallable, CallInEditor, Category="Patrol")
	void ClearPatrolPoints();
	
	// 몹 스포너 ID 생성. PCG 그래프로 Spawn 될 때 1회 실행됨. 이후 Collect 마다 추가 호출
	UFUNCTION(BlueprintCallable, CallInEditor, Category="Patrol")
	void EnsureMobSpawnerID();
	
private:
	UPROPERTY(EditAnywhere)
	FName MobSpawnerID;
	
	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<AERNEnemyCharacter>> ActiveEnemiesBySlot;
	
	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<UMobSpawnPointComponent>> CachedSpawnPoints;
	
	// 월드에 배치된 패트롤 TargetPoint. PCG_MobPatrolPoint 로 생성함.
	UPROPERTY(EditAnywhere, Category = "Patrol", meta = (AllowPrivateAccess = "true"))
	TArray<TObjectPtr<AActor>> PatrolTargetPoints;
private:
	// 저장된 데이터가 없는 경우의 몬스터 스폰
	void SpawnInitialMobs();
	// 몬스터 스폰 및 상태 조정을 위한 포인터 반환
	AERNEnemyCharacter* SpawnMobFromPoint(UMobSpawnPointComponent* SpawnPoint);
	// 저장된 데이터를 기반으로 몹 스폰 밎 상태 조정
	void RestoreMobFromState(const FMobRuntimeState& State);
	// 몹들의 상태를 기록하고 월드 매니저에 저장 요청
	void SaveMobStates();
	// 저장 이후 몹들 Destroy
	void ClearActiveEnemies();
	
	// 부착된 UMobSpawnPointComponent 들 수집, 캐시화
	void CachingSpawnPoints();
};
