// Fill out your copyright notice in the Description page of Project Settings.

#include "World/StructureSpawnService.h"

#include "World/Data/StructureSpawnConfigDataAsset.h"

#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/TargetPoint.h"

void UStructureSpawnService::SpawnStructures(UWorld* World, const UStructureSpawnConfigDataAsset* SpawnConfig)
{
	if (World == nullptr || SpawnConfig == nullptr)
	{
		return;
	}
	
	// 실제 레벨에 배치된 타겟 포인터를 순회하며 해당 지점에 건물 배치
	TArray<ATargetPoint*> TargetPoints;
	
	for (TActorIterator<ATargetPoint> It(World); It; ++It)
	{
		ATargetPoint* TargetPoint = *It;
		if (TargetPoint == nullptr)
		{
			continue;
		}
		
		TargetPoints.Add(TargetPoint);
	}
	
	// 건물 스폰
	if (TargetPoints.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("TargetPoint 가 배치되어 있지 않습니다. 건물 생성을 중단합니다."));
		return;
	}
	
	// 스폰될 건물 목록 순회
	for (const FERNStructureSpawnEntry& Entry : SpawnConfig->SpawnEntries)
	{
		if (Entry.PackedLevelActorClass.IsNull())
		{
			continue;
		}
		
		UClass* PackedClass = Entry.PackedLevelActorClass.LoadSynchronous();
		if (PackedClass == nullptr)
		{
			continue;
		}
		
		const int32 MinCount = FMath::Max(0, Entry.MinSpawnCount);
		const int32 MaxCount = FMath::Max(MinCount, Entry.MaxSpawnCount);
		const int32 SpawnCount = FMath::RandRange(MinCount, MaxCount);
		
		// 각 건물 종류에 대해 최소 ~ 최대 사이의 갯수로 중복 위치 없이 스폰
		for (int32 SpawnIndex = 0; SpawnIndex < SpawnCount && !TargetPoints.IsEmpty(); ++SpawnIndex)
		{
			const int32 TargetIndex = FMath::RandRange(0, TargetPoints.Num() - 1);
			ATargetPoint* TargetPoint = TargetPoints[TargetIndex];
			TargetPoints.RemoveAtSwap(TargetIndex);
			
			if (TargetPoint == nullptr)
			{
				continue;
			}
			
			// 항상 스폰 되도록 설정
			FActorSpawnParameters SpawnParams;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			
			APackedLevelActor* SpawnedActor = World->SpawnActor<APackedLevelActor>(PackedClass,TargetPoint->GetActorTransform(),SpawnParams);
			
			if (SpawnedActor == nullptr)
			{
				continue;
			}
			
			// 멀티 플레이 환경에서 복제
			SpawnedActor->SetReplicates(true);
		}
	}
}

// 스태틱 메시 버전 생성 함수
/*
void UStructureSpawnService::SpawnStructures(UWorld* World, const UStructureSpawnConfigDataAsset* SpawnConfig)
{
	if (World == nullptr || SpawnConfig == nullptr)
	{
		return;
	}
	
	// 실제 레벨에 배치된 타겟 포인터를 순회하며 해당 지점에 건물 배치
	TArray<ATargetPoint*> TargetPoints;
	
	for (TActorIterator<ATargetPoint> It(World); It; ++It)
	{
		ATargetPoint* TargetPoint = *It;
		if (TargetPoint == nullptr)
		{
			continue;
		}
		
		TargetPoints.Add(TargetPoint);
	}
	
	// 건물 스폰
	if (TargetPoints.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("TargetPoint 가 배치되어 있지 않습니다. 건물 생성을 중단합니다."));
		return;
	}
	
	// 스폰될 건물 목록 순회
	for (const FERNStructureSpawnEntry& Entry : SpawnConfig->SpawnEntries)
	{
		if (Entry.StaticMesh.IsNull())
		{
			continue;
		}
		
		UStaticMesh* StaticMesh = Entry.StaticMesh.LoadSynchronous();
		if (StaticMesh == nullptr)
		{
			continue;
		}
		
		const int32 MinCount = FMath::Max(0, Entry.MinSpawnCount);
		const int32 MaxCount = FMath::Max(MinCount, Entry.MaxSpawnCount);
		const int32 SpawnCount = FMath::RandRange(MinCount, MaxCount);
		
		// 각 건물 종류에 대해 최소 ~ 최대 사이의 갯수로 중복 위치 없이 스폰
		for (int32 SpawnIndex = 0; SpawnIndex < SpawnCount && !TargetPoints.IsEmpty(); ++SpawnIndex)
		{
			const int32 TargetIndex = FMath::RandRange(0, TargetPoints.Num() - 1);
			ATargetPoint* TargetPoint = TargetPoints[TargetIndex];
			TargetPoints.RemoveAtSwap(TargetIndex);
			
			if (TargetPoint == nullptr)
			{
				continue;
			}
			
			// 항상 스폰 되도록 설정
			FActorSpawnParameters SpawnParams;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			
			AStaticMeshActor* SpawnedActor = World->SpawnActor<AStaticMeshActor>(	AStaticMeshActor::StaticClass(), 
																					TargetPoint->GetActorTransform(), 
																					SpawnParams);
			
			if (SpawnedActor == nullptr)
			{
				continue;
			}
			
			// 멀티 플레이 환경에서 복제
			SpawnedActor->SetReplicates(true);
			
			// 껍데기에 실제 데이터 에셋에 들어있는 원본 건물의 스태틱 메시를 주입
			UStaticMeshComponent* MeshComponent = SpawnedActor->GetStaticMeshComponent();
			if (MeshComponent)
			{
				MeshComponent->SetStaticMesh(StaticMesh);
			}
		}
	}
}
*/