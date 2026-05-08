// Fill out your copyright notice in the Description page of Project Settings.

#include "World/StructureSpawnService.h"

#include "World/Data/StructureSpawnConfigDataAsset.h"

#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "World/StructureSpawnPoint.h"

// 건물 랜덤 위치 생성 함수. 월드 입장 후 1회만 실행
void UStructureSpawnService::SpawnStructures(UWorld* World, const UStructureSpawnConfigDataAsset* SpawnConfig)
{
	if (World == nullptr || SpawnConfig == nullptr)
	{
		return;
	}
	
	// 스폰 가능 지점 수집
	CollectSpawnPoints(World);
	
	// 건물 종류별 최소 개수만큼 SpawnEntryStructures 호출
	SpawnMinimumStructures(World, SpawnConfig);
	
	// 최소 스폰 이후 추가 생성
	SpawnAdditionalStructures(World, SpawnConfig);
}

void UStructureSpawnService::CollectSpawnPoints(UWorld* World)
{
	SpawnPoints.Reset();
	
	if (World == nullptr)
	{
		return;
	}
	
	// 실제 레벨에 배치된 AStructureSpawnPoint를 순회하고 저장
	for (TActorIterator<AStructureSpawnPoint> It(World); It; ++It)
	{
		AStructureSpawnPoint* TargetPoint = *It;
		if (TargetPoint == nullptr)
		{
			continue;
		}
		
		// 해당 포인트에 지정된 구조물 타입에 따라 Map에 분류하여 저장
		SpawnPoints.FindOrAdd(TargetPoint->StructureType).Add(TargetPoint);
	}
}

void UStructureSpawnService::SpawnMinimumStructures(UWorld* World, const UStructureSpawnConfigDataAsset* SpawnConfig)
{
	// 스폰될 건물 목록 순회
	for (const FERNStructureSpawnEntry& Entry : SpawnConfig->SpawnEntries)
	{
		if (Entry.PackedLevelActorClass.IsNull())
		{
			continue;
		}
		
		if(Entry.MinSpawnCount > Entry.MaxSpawnCount)
		{
			UE_LOG(LogTemp, Error, TEXT("건물의 최소 생성 갯수가 최대 생성 갯수보다 크게 잡혀있습니다. %s"), *Entry.PackedLevelActorClass.ToString());
		}
		
		int32 SpawnResult = SpawnEntryStructures(World, Entry, Entry.MinSpawnCount );
		if (SpawnResult < Entry.MinSpawnCount)
		{
			UE_LOG(LogTemp,Warning,TEXT("최소 스폰 갯수만큼 스폰하지 못한 건물이 존재합니다. %s"), *Entry.PackedLevelActorClass.ToString());
		}
	}
}

void UStructureSpawnService::SpawnAdditionalStructures(UWorld* World, const UStructureSpawnConfigDataAsset* SpawnConfig)
{
	// 스폰될 건물 목록 순회
	for (const FERNStructureSpawnEntry& Entry : SpawnConfig->SpawnEntries)
	{
		if (Entry.PackedLevelActorClass.IsNull())
		{
			continue;
		}
		
		const int32 MaxCount = FMath::Max(0, Entry.MaxSpawnCount - Entry.MinSpawnCount);
		const int32 SpawnCount = FMath::RandRange(0, MaxCount);
		SpawnEntryStructures(World, Entry, SpawnCount);
	}
}

int32 UStructureSpawnService::SpawnEntryStructures(UWorld* World, const FERNStructureSpawnEntry& Entry, int32 DesiredSpawnCount)
{
	if (DesiredSpawnCount == 0)
	{
		return 0;
	}
	
	if (World == nullptr || Entry.PackedLevelActorClass.IsNull())
	{
		return -1;
	}
	
	UClass* PackedClass = Entry.PackedLevelActorClass.LoadSynchronous();
	if (PackedClass == nullptr)
	{
		return -1;
	}
	
	// 생성해도 되는 최대치까지 비어있는 공간에 건물 스폰 시도
	int32 SpawnSuccessCnt = 0;
	
	while (SpawnSuccessCnt < DesiredSpawnCount)
	{
		AStructureSpawnPoint* SpawnPoint = PopRandomSpawnPoint(Entry.SpawnType);
		if (SpawnPoint == nullptr)
		{
			break;
		}
		
		if (TrySpawnStructureAtPoint(World, PackedClass, SpawnPoint))
		{
			SpawnSuccessCnt++;
		}
	}
	
	// 빈 공간이 없다면 SpawnSuccessCnt < DesiredSpawnCount 가능 
	return SpawnSuccessCnt;
}

bool UStructureSpawnService::TrySpawnStructureAtPoint(UWorld* World, UClass* PackedClass, const AStructureSpawnPoint* SpawnPoint)
{
	if (World == nullptr || PackedClass == nullptr || SpawnPoint == nullptr)
	{
		UE_LOG(LogTemp, Error, TEXT("UStructureSpawnService::TrySpawnStructureAtPoint 매개변수 유효성 인증 실패"));
		return false;
	}

	// 항상 스폰 되도록 설정, 스폰
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	APackedLevelActor* SpawnedActor = World->SpawnActor<APackedLevelActor>(PackedClass, SpawnPoint->GetActorTransform(), SpawnParams);

	if (SpawnedActor == nullptr)
	{
		return false;
	}

	// 멀티 플레이 환경에서 복제
	SpawnedActor->SetReplicates(true);
	
	return true;
}

AStructureSpawnPoint* UStructureSpawnService::PopRandomSpawnPoint(EStructureType SpawnType)
{
	// 원본의 포인터를 접근해서 수정
	TArray<AStructureSpawnPoint*>* Candidates = SpawnPoints.Find(SpawnType);
	if (Candidates == nullptr || Candidates->IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("UStructureSpawnService::PopRandomSpawnPoint 생성하려는 건물 타입에 맞는 남은 스폰 포인트가 없습니다."));
		return nullptr;
	}

	const int32 Index = FMath::RandRange(0, Candidates->Num() - 1);
	AStructureSpawnPoint* SelectedPoint = (*Candidates)[Index];
	Candidates->RemoveAtSwap(Index);

	return SelectedPoint;
}