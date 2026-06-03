// Fill out your copyright notice in the Description page of Project Settings.


#include "World/MobSpawner.h"

#include "ERNWorldManagerSubsystem.h"
#include "MobPatrolPoint.h"
#include "MobSpawnPointComponent.h"
#include "Character/Enemy/ERNEnemyCharacter.h"
#include "GAS/ERNAttributeSet.h"
#include "Kismet/GameplayStatics.h"
#include "World/Data/MobRuntimeState.h"

// Sets default values
AMobSpawner::AMobSpawner()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;

}

// Called when the game starts or when spawned
void AMobSpawner::BeginPlay()
{
	Super::BeginPlay();
	
	if (HasAuthority() == false)
	{
		return;
	}
	
	if (MobSpawnerID.IsNone() == true)
	{
		UE_LOG(LogTemp, Warning, TEXT("MobSpawner ID is None"));
		return;
	}
	
	// 수집된 컴포넌트 캐시화
	CachingSpawnPoints();
	
	if (CachedSpawnPoints.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("AMobSpawner::BeginPlay no valid spawn points. Spawner: %s"), *GetName());
		return;
	}
	
	UERNWorldManagerSubsystem* WorldManager = GetWorld()->GetSubsystem<UERNWorldManagerSubsystem>();
	if (WorldManager == nullptr)
	{
		// ERN 월드 매니저가 없어도 스폰은 항상 가능
		SpawnInitialMobs();
		return;
	}
	
	// 스포너에 저장된 데이터를 월드 매니저에서 가져오기
	TArray<FMobRuntimeState> SavedStates;
	const bool bHasSavedStates = WorldManager->TryGetMobStates(MobSpawnerID, SavedStates);
	
	// 저장된 데이터가 있다면 저장된 상태로 복원하고 스폰
	if (bHasSavedStates)
	{
		for (const FMobRuntimeState& State : SavedStates)
		{
			RestoreMobFromState(State);
		}
		return;
	}
	else
	{
		SpawnInitialMobs();
	}
	return;
}

void AMobSpawner::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (HasAuthority() == false)
	{
		Super::EndPlay(EndPlayReason);
		return;
	}
	
	if (EndPlayReason == EEndPlayReason::RemovedFromWorld)
	{
		SaveMobStates();
		ClearActiveEnemies();
	}
	
	Super::EndPlay(EndPlayReason);
}

void AMobSpawner::CollectPatrolPoints()
{
	EnsureMobSpawnerID();
	Modify();
	
	PatrolTargetPoints.Empty();
	
	// 자식 엑터 중에서 부착된 포인트 수집, 연결
	TArray<AActor*> AttachedActors;
	GetAttachedActors(AttachedActors, true, true);
	
	for (AActor* Actor : AttachedActors)
	{
		AMobPatrolPoint* Point = Cast<AMobPatrolPoint>(Actor);
		if (Point == nullptr)
		{
			continue;
		}
		
		Point->Modify();
		Point->OwningSpawnerID = MobSpawnerID;
		Point->OwningSpawner = this;
		PatrolTargetPoints.AddUnique(Point);
	}
	
	// 부착되지 않은 연결된 엑터 수집
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return;
	}
	
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(World, AMobPatrolPoint::StaticClass(), FoundActors);
	
	for (AActor* Actor : FoundActors)
	{
		// 본인 소속이 아니라면 생략. 소속은 PCG에서 생성될 때 자동으로 할당됨
		AMobPatrolPoint* PatrolPoint = Cast<AMobPatrolPoint>(Actor);
		if (PatrolPoint == nullptr || PatrolPoint->OwningSpawner != this)
		{
			continue;
		}
		
		PatrolTargetPoints.AddUnique(PatrolPoint);
	}
}

void AMobSpawner::ClearPatrolPoints()
{
	Modify();
	PatrolTargetPoints.Empty();
}

void AMobSpawner::EnsureMobSpawnerID()
{
	if (MobSpawnerID.IsNone() == false)
	{
		return;
	}
	
	//몹 스포너 ID 비어 있으면 할당
	Modify();
}

void AMobSpawner::SpawnInitialMobs()
{
	if (CachedSpawnPoints.IsEmpty())
	{
		return;
	}
	
	// 캐시된 스폰 포인트들을 순회하며 해당 지점에 몬스터 스폰
	for (const TPair<FName, TObjectPtr<UMobSpawnPointComponent>>& Pair : CachedSpawnPoints)
	{
		TObjectPtr<UMobSpawnPointComponent> SpawnPoint = Pair.Value.Get();
		if (SpawnPoint == nullptr)
		{
			UE_LOG(LogTemp, Error, TEXT("SpawnMobs의 SpawnPoint가 존재하지 않습니다."));
			continue;
		}
		SpawnMobFromPoint(SpawnPoint);
	}
	
}

AERNEnemyCharacter* AMobSpawner::SpawnMobFromPoint(UMobSpawnPointComponent* SpawnPoint)
{
	if (SpawnPoint == nullptr)
	{
		return nullptr;
	}
	
	if (SpawnPoint->SlotId.IsNone() || SpawnPoint->EnemyClass == nullptr)
	{
		return nullptr;
	}
	
	//State.SlotId로 CachedSpawnPoints를 찾아 EnemyClass를 가져온 뒤 State.Transform 위치에 스폰하고 CurrentHealth
	TSubclassOf<AERNEnemyCharacter> EnemyClass = SpawnPoint->EnemyClass;

	if (EnemyClass == nullptr)
	{
		return nullptr;
	}
	
	if (GetWorld() == nullptr)
	{
		return nullptr;
	}

	
	if (ActiveEnemiesBySlot.Contains(SpawnPoint->SlotId))
	{
		UE_LOG(LogTemp, Warning, TEXT("Enemy already spawned. SlotId: %s"), *SpawnPoint->SlotId.ToString());
		return ActiveEnemiesBySlot[SpawnPoint->SlotId];
	}
	
	FActorSpawnParameters SpawnParams;
	AERNEnemyCharacter* SpawnedEnemy = GetWorld()->SpawnActor<AERNEnemyCharacter>(SpawnPoint->EnemyClass,SpawnPoint->GetComponentTransform(), SpawnParams);
	
	if (SpawnedEnemy == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("SpawnedEnemy is nullptr failed spawn enemy"));
		return nullptr; 
	}
	
	// 스폰한 몹에게 패트롤 포인트 전달
	SpawnedEnemy->SetPatrolPoints(PatrolTargetPoints);
	
	
	// 스폰된 적 활성화된 적으로 추가
	ActiveEnemiesBySlot.Add(SpawnPoint->SlotId, SpawnedEnemy);
	
	UWorld* World = GetWorld();
	if (World == nullptr)
	{
		return nullptr;
	}

	if (ActiveEnemiesBySlot.Contains(SpawnPoint->SlotId))
	{
		UE_LOG(LogTemp, Warning, TEXT("Enemy already spawned. SlotId: %s"), *SpawnPoint->SlotId.ToString());
		return ActiveEnemiesBySlot[SpawnPoint->SlotId];
	}
	
	return SpawnedEnemy;
	
}

void AMobSpawner::RestoreMobFromState(const FMobRuntimeState& State)
{
	if (State.bDead == true)
	{
		//사망한 대상이면 생략
		return;
	}
	TObjectPtr<UMobSpawnPointComponent>* FoundSpawnPoint = CachedSpawnPoints.Find(State.SlotId);
	
	// 저장된 데이터 유효성 검증
	if (FoundSpawnPoint == nullptr || FoundSpawnPoint->Get() == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("RestoreMobFromState failed. SlotId not found: %s"), *State.SlotId.ToString());
		return;
	}
	
	// 스폰 포인트에 몹 스폰. 위치는 초기에 레벨에 배치된 위치로 스폰
	UMobSpawnPointComponent* SpawnPoint = FoundSpawnPoint->Get();
	AERNEnemyCharacter* SpawnedEnemy = SpawnMobFromPoint(SpawnPoint);
	
	if (SpawnedEnemy == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("Restore enemy health failed. SpawnedEnemy is nullptr : %s"), *SpawnPoint->GetName());
		return;
	}
	
	// 소환된 몹 현재 체력을 저장된 체력으로 변경
	UERNAttributeSet* AttributeSet = SpawnedEnemy->GetAttributeSet();
	if (AttributeSet)
	{
		AttributeSet->SetHealth(FMath::Clamp(State.CurrentHealth,0.f, AttributeSet->GetMaxHealth()));
	}
}

void AMobSpawner::SaveMobStates()
{
	UERNWorldManagerSubsystem* WorldManagerSubsystem = GetWorld() ? GetWorld()->GetSubsystem<UERNWorldManagerSubsystem>() : nullptr;
	
	if (WorldManagerSubsystem == nullptr)
	{
		return;
	}
	
	TArray<FMobRuntimeState> MobStates;
	MobStates.Reserve(CachedSpawnPoints.Num());
	
	// 몹 상태 수집
	for (const TPair<FName, TObjectPtr<UMobSpawnPointComponent>>& Pair : CachedSpawnPoints)
	{
		const FName SlotId = Pair.Key;
		FMobRuntimeState CurrentState;
		CurrentState.SlotId = SlotId;

		// 현재 수집중인 스폰 포인트에 들어있는 몹이 활성화 상태인지 확인
		TObjectPtr<AERNEnemyCharacter>* FoundEnemy = ActiveEnemiesBySlot.Find(SlotId);
		AERNEnemyCharacter* Enemy = FoundEnemy ? FoundEnemy->Get() : nullptr;
		
		
		// 제거되었거나 죽고 있는 몹은 사망처리로 기록
		if (Enemy == nullptr || Enemy->IsActorBeingDestroyed())
		{
			CurrentState.bDead = true;
			MobStates.Add(CurrentState);
			continue;
		}

		else
		{
			CurrentState.Transform = Enemy->GetActorTransform();
			CurrentState.bDead = false;
			
			if (UERNAttributeSet* AttributeSet = Enemy->GetAttributeSet())
			{
				// 제거된건 아니지만 어차피 사망 후 제거될 엑터도 사망 처리로 기록
				CurrentState.CurrentHealth = AttributeSet->GetHealth();
				
				if (Enemy->IsDead())
				{
					CurrentState.bDead = true;
				}
			}
			MobStates.Add(CurrentState);
		}
	}
	
	// 수집된 몹 상태들을 월드 매니저로 전송 및 저장
	WorldManagerSubsystem->SaveMobStates(MobSpawnerID, MobStates);
}

void AMobSpawner::ClearActiveEnemies()
{
	if (ActiveEnemiesBySlot.IsEmpty())
	{
		return;
	}

	for (const TPair<FName, TObjectPtr<AERNEnemyCharacter>>& Pair : ActiveEnemiesBySlot)
	{
		AERNEnemyCharacter* Enemy = Pair.Value.Get();
		if (Enemy == nullptr || Enemy->IsActorBeingDestroyed())
		{
			continue;
		}

		Enemy->Destroy();
	}

	ActiveEnemiesBySlot.Empty();
}

void AMobSpawner::CachingSpawnPoints()
{
	CachedSpawnPoints.Empty();
	
	TArray<UMobSpawnPointComponent*> MobSpawnPoints;
	GetComponents<UMobSpawnPointComponent>(MobSpawnPoints);
	
	for (UMobSpawnPointComponent* SpawnPoint : MobSpawnPoints)
	{
		// 유효성 검사
		if (SpawnPoint == nullptr)
		{
			UE_LOG(LogTemp, Warning, TEXT("SpawnPoint is NULL"));
			continue;
		}
		
		if (SpawnPoint->SlotId.IsNone())
		{
			UE_LOG(LogTemp, Warning, TEXT("SpawnPoint->SlotId is None"));
			continue;
		}
		
		if (SpawnPoint->EnemyClass == nullptr)
		{
			UE_LOG(LogTemp, Warning, TEXT("SpawnPoint->EnemyClass is None"));
			continue;
		}
		
		// 중복 검사
		if (CachedSpawnPoints.Contains(SpawnPoint->SlotId))
		{
			UE_LOG(LogTemp, Warning, TEXT("SpawnPoint->SlotId Already Exists"));
			continue;
		}
		
		// 검증된 데이터 추가
		CachedSpawnPoints.Add(SpawnPoint->SlotId, SpawnPoint);
	}
}

