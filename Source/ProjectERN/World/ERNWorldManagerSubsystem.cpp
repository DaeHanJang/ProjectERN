// Fill out your copyright notice in the Description page of Project Settings.


#include "World/ERNWorldManagerSubsystem.h"

#include "ERNDayNightCycleService.h"
#include "ERNWorldManagerSettings.h"
#include "MobSpawner.h"
#include "StructureSpawnService.h"
#include "Core/ERNGameState.h"
#include "Data/ERNDayNightCycleConfigDataAsset.h"
#include "Data/ERNWorldTable.h"
#include "Data/StructureSpawnConfigDataAsset.h"

bool UERNWorldManagerSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	if (Super::ShouldCreateSubsystem(Outer) == false)
	{
		return false;
	}
	
	//데이터 테이블에서 현재 맵이 포함되어 있다면 WorldManager를 사용
	const UWorld* World = Cast<UWorld>(Outer);
	FERNWorldTableRow FoundRow;
	return TryFindWorldRow(World, FoundRow, nullptr);
}

void UERNWorldManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	//다시 데이터 테이블 조회하고 이번엔 데이터 에셋 가져와서 캐싱까지 처리
	FERNWorldTableRow FoundRow;
	if (false == TryFindWorldRow(GetWorld(), FoundRow, &CurrentWorldPath))
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to cache world row for ERNWorldManagerSubsystem."));
		return;
	}

	//데이터 에셋 (SpawnConfig)를 기반으로 건물 생성을 처리할 Object 생성
	CachedSpawnConfig = FoundRow.SpawnConfig;
	StructureSpawnService = NewObject<UStructureSpawnService>(this);
	
	//마찬가지로 시간 흐름을 처리할 Object 생성
	CachedDayNightConfig = FoundRow.DayNightConfig;
	DayNightCycleService = NewObject<UERNDayNightCycleService>(this);
}

void UERNWorldManagerSubsystem::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	
	// 낮밤 서비스 초기화는 서버/클라 모두 해야 함
	if (DayNightCycleService && !CachedDayNightConfig.IsNull())
	{
		DayNightCycleService->Initialize(
			&InWorld,
			CachedDayNightConfig.LoadSynchronous()
		);
	}
	
	//서버 권한 확인
	//HasAuthority 를 사용하지 못하는 관계상, 클라이언트는 조기 종료.
	if (InWorld.GetNetMode() == NM_Client)
	{
		return;
	}
	
	//SpawnConfig 로드 -> StructureSpawnService에게 스폰 요청
	UStructureSpawnConfigDataAsset* SpawnConfig = CachedSpawnConfig.LoadSynchronous();
	if (SpawnConfig && StructureSpawnService)
	{
		StructureSpawnService->SpawnStructures(&InWorld, SpawnConfig);
	}
	
	
	// 낮밤 시간 흐름 시작 명령
	if (AERNGameState* GameState = InWorld.GetGameState<AERNGameState>())
	{
		GameState->StartDayNightCycle(CachedDayNightConfig->DayToNightDuration);
	}
	
}

// 스포너 ID에 대해 저장된 데이터가 있는지 확인해서 넣어주는 함수
bool UERNWorldManagerSubsystem::TryGetMobStates(FName SpawnerId, TArray<FMobRuntimeState>& OutSavedStates) const
{
	if (SpawnerId.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("SpawnerId is null"));
		return false;
	}

	OutSavedStates.Reset();
	
	//SpawnerID 를 TMap에서 찾아서 넣어주기
	const FSpawnerMobRuntimeStates* FoundStates = SpawnerMobStates.Find(SpawnerId);
	if (FoundStates == nullptr)
	{
		return false;
	}
	
	OutSavedStates = FoundStates->MobStates;
	return true;
}

void UERNWorldManagerSubsystem::SaveMobStates(FName SpawnerId, const TArray<FMobRuntimeState>& InStates)
{
	if (SpawnerId.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("SpawnerId is null"));
		return;
	}
	
	FSpawnerMobRuntimeStates& SaveStates = SpawnerMobStates.FindOrAdd(SpawnerId);
	SaveStates.MobStates = InStates;
}


// 현재 맵을 월드 세팅 매니저의 데이터 테이블에서 찾아내는 공용 함수
bool UERNWorldManagerSubsystem::TryFindWorldRow(const UWorld* World, FERNWorldTableRow& OutWorldRow, FString* OutWorldPath) const
{
	if (World == nullptr)
	{
		return false;
	}

	const UERNWorldManagerSettings* Settings = GetDefault<UERNWorldManagerSettings>();
	if (Settings == nullptr || Settings->WorldTable.IsNull())
	{
		return false;
	}

	const UDataTable* WorldTable = Settings->WorldTable.LoadSynchronous();
	if (WorldTable == nullptr)
	{
		return false;
	}

	// 에디터에서 실행하는 경우만 불필요한 이름 제거.
	const FString WorldPath =
		UWorld::RemovePIEPrefix(World->GetOutermost()->GetName());

	if (OutWorldPath != nullptr)
	{
		*OutWorldPath = WorldPath;
	}

	TArray<FERNWorldTableRow*> Rows;
	WorldTable->GetAllRows<FERNWorldTableRow>(
		TEXT("UERNWorldManagerSubsystem::TryFindWorldRow"),
		Rows
	);

	// ERNWorldManagerSetting 에 있는 데이터 테이블에 현재 맵이 포함되어 있는지 확인
	for (const FERNWorldTableRow* Row : Rows)
	{
		if (Row == nullptr || Row->World.IsNull())
		{
			continue;
		}

		const FString RegisteredWorldPath =
			Row->World.ToSoftObjectPath().GetLongPackageName();

		if (WorldPath == RegisteredWorldPath)
		{
			OutWorldRow = *Row; // 포인터 캐싱 대신 값 복사
			return true;
		}
	}

	return false;
}
