// Fill out your copyright notice in the Description page of Project Settings.

#include "Inventory/Item/Manager/ItemManagerSubsystem.h"

#include "Engine/AssetManager.h"
#include "Inventory/Item/Data/ItemDataAssetBase.h"

void UItemManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	// 아이템 테이블은 핵심 데이터이기 때문에 꼭 존재해야 함
	checkf(ItemTable, TEXT("ItemTable is not assigned in ItemManagerSubsystem."));
}

void UItemManagerSubsystem::Deinitialize()
{
	for (TPair<FName, FPendingItemDataAssetLoad>& Pair : PendingItemDataAssetLoads)
	{
		// 비동기 로드중인 요청 정리
		if (Pair.Value.Handle.IsValid())
		{
			Pair.Value.Handle->CancelHandle();
		}
		Pair.Value.RequestedLoadFlags = EItemAssetLoadFlags::None;
	}
	
	PendingItemDataAssetLoads.Empty();
	ItemDataAssetCache.Empty();
	
	Super::Deinitialize();
}

bool UItemManagerSubsystem::ItemValid(const FName ItemID) const
{
	// 서버 검증
	if (!CanAccessItemTable())
	{
		return false;
	}
	
	return FindItemRow(ItemID) != nullptr;
}

bool UItemManagerSubsystem::CanAccessItemTable() const
{
	const UWorld* World = GetWorld();
	// 월드 생성 이후 사용 가능, 서버 전용
	if (!World || World->GetNetMode() == ENetMode::NM_Client)
	{
		return false;
	}
	
	return true;
}

const FERNItemTable* UItemManagerSubsystem::FindItemRow(const FName ItemID) const
{
	// 서버 검증
	if (!CanAccessItemTable())
	{
		return nullptr;
	}
	
	// ItemTable이 존재하는지 체크 (Shipping에서는 checkf가 사라지기 때문에 한 번 더 체크)
	if (!ensureMsgf(ItemTable, TEXT("ItemTable is null in FindItemRow.")))
	{
		return nullptr;
	}
	// ItemID 유효한지 체크
	if (ItemID.IsNone())
	{
		return nullptr;
	}
	
	return ItemTable->FindRow<FERNItemTable>(ItemID, TEXT("UItemManagerSubsystem::FindItemRow"));
}

const UItemDataAssetBase* UItemManagerSubsystem::LoadItemDataAssetSync(const FName ItemID, const EItemAssetLoadFlags LoadFlags)
{
	// 서버 검증
	if (!CanAccessItemTable())
	{
		return nullptr;
	}
	
	const FERNItemTable* Row = FindItemRow(ItemID);
	// ItemID에 해당하는 테이블 행이 없거나 경로가 존재하지 않는 경우
	if (!Row || Row->DataAsset.IsNull())
	{
		return nullptr;
	}
	// 로드 정책이 유효하지 않을 경우 
	if (LoadFlags == EItemAssetLoadFlags::None)
	{
		return nullptr;
	}
	
	// 요청받은 아이템 키가 캐싱되어 있다면 캐시 데이터 반환
	if (const TObjectPtr<const UItemDataAssetBase>* Cached = ItemDataAssetCache.Find(ItemID))
	{
		if (const UItemDataAssetBase* CachedDataAsset = Cached->Get())
		{
			TArray<FSoftObjectPath> NestedPaths;
			CachedDataAsset->GatherSoftPaths(LoadFlags, NestedPaths);
			
			if (!NestedPaths.IsEmpty())
			{
				UAssetManager::GetStreamableManager().RequestSyncLoad(NestedPaths);
			}
			
			return CachedDataAsset;
		}
	}
	
	// 아이템 데이터 애셋 동기 로드
	UItemDataAssetBase* LoadedDataAsset = Cast<UItemDataAssetBase>(
		UAssetManager::GetStreamableManager().LoadSynchronous(
			Row->DataAsset.ToSoftObjectPath(), false
		)
	);
	if (!LoadedDataAsset)
	{
		return nullptr;
	}
	
	// 캐싱
	ItemDataAssetCache.Add(ItemID, LoadedDataAsset);
	
	TArray<FSoftObjectPath> NestedPaths;
	// 데이터 애셋에 존재하는 Soft reference 리소스 경로 추출
	LoadedDataAsset->GatherSoftPaths(LoadFlags, NestedPaths);
	
	if (!NestedPaths.IsEmpty())
	{
		// 데이터 애셋에 정의된 리소스 동기 로드
		UAssetManager::GetStreamableManager().RequestSyncLoad(NestedPaths);
	}
	
	return LoadedDataAsset;
}

void UItemManagerSubsystem::PreloadItemDataAssetAsync(const FName ItemID, const EItemAssetLoadFlags LoadFlags)
{
	// 서버 검증
	if (!CanAccessItemTable())
	{
		return;
	}
	
	const FERNItemTable* Row = FindItemRow(ItemID);
	// ItemID에 해당하는 테이블 행이 없거나 경로가 존재하지 않는 경우
	if (!Row || Row->DataAsset.IsNull())
	{
		return;
	}
	// 로드 정책이 유효하지 않을 경우 
	if (LoadFlags == EItemAssetLoadFlags::None)
	{
		return;
	}
	
	// 요청받은 아이템 키가 캐싱되어 있다면 데이터 애셋에 정의된 리소스 비동기 로드
	if (ItemDataAssetCache.Contains(ItemID))
	{
		if (const UItemDataAssetBase* CachedDataAsset = ItemDataAssetCache[ItemID])
		{
			TArray<FSoftObjectPath> NestedPaths;
			CachedDataAsset->GatherSoftPaths(LoadFlags, NestedPaths);
			
			if (!NestedPaths.IsEmpty())
			{
				UAssetManager::GetStreamableManager().RequestAsyncLoad(NestedPaths);
			}
		}
		
		return;
	}
	
	// 비동기 로드 요청 테이블에 요청된 ItemID를 찾고 로드 정책만 누적 없으면 추가
	FPendingItemDataAssetLoad& PendingLoad = PendingItemDataAssetLoads.FindOrAdd(ItemID);
	PendingLoad.RequestedLoadFlags |= LoadFlags;
	
	// 이미 DataAsset 비동기 로드가 진행 중이라면 중복 요청 방지
	if (PendingLoad.Handle.IsValid())
	{
		return;
	}
	
	const TSoftObjectPtr<const UItemDataAssetBase> DataAssetRef = Row->DataAsset;
	// 비동기 로드가 나중에 실행될 수 있기 때문에 약한 참조
	TWeakObjectPtr<UItemManagerSubsystem> WeakThis(this);
	
	// 아이템 데이터 애셋 비동기 로드
	PendingLoad.Handle = UAssetManager::GetStreamableManager().RequestAsyncLoad(
		DataAssetRef.ToSoftObjectPath(),
		FStreamableDelegate::CreateLambda([WeakThis, ItemID, DataAssetRef]()
		{
			if (!WeakThis.IsValid())
			{
				return;
			}
			
			// 비동기 로드 요청 추적 테이블에서 이 요청 객체를 가져옴
			FPendingItemDataAssetLoad* PendingLoadPtr = WeakThis->PendingItemDataAssetLoads.Find(ItemID);
			if (!PendingLoadPtr)
			{
				return;
			}
			
			// 로드 중 누적된 최종 플래그를 가져옴
			const EItemAssetLoadFlags RequestedFlags = PendingLoadPtr->RequestedLoadFlags;
			
			if (PendingLoadPtr->Handle.IsValid())
			{
				// 이 비동기 요청 핸들 제거
				PendingLoadPtr->Handle->ReleaseHandle();
			}
			
			// 비동기 로드 요청 추적 테이블에서 제거
			WeakThis->PendingItemDataAssetLoads.Remove(ItemID);
			
			// 비동기 로드 완료된 데이터 애셋 가져오기
			const UItemDataAssetBase* LoadedDataAsset = DataAssetRef.Get();
			if (!LoadedDataAsset)
			{
				return;
			}
			
			// 캐싱
			WeakThis->ItemDataAssetCache.Add(ItemID, LoadedDataAsset);
			
			TArray<FSoftObjectPath> NestedPaths;
			// 데이터 애셋에 존재하는 Soft reference 리소스 경로 추출
			LoadedDataAsset->GatherSoftPaths(RequestedFlags, NestedPaths);
			
			if (!NestedPaths.IsEmpty())
			{
				// 데이터 애셋에 정의된 리소스 비동기 로드
				UAssetManager::GetStreamableManager().RequestAsyncLoad(NestedPaths);
			}
		})
	);
}
