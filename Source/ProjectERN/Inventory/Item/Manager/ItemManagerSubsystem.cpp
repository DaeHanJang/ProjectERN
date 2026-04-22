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

bool UItemManagerSubsystem::ItemValid(const FName ItemID) const
{
	const UWorld* World = GetWorld();
	// 월드 생성 이후 사용 가능, 서버 전용
	if (!World || World->GetNetMode() == ENetMode::NM_Client)
	{
		return nullptr;
	}
	
	return FindItemRow(ItemID) != nullptr;
}

const FERNItemTable* UItemManagerSubsystem::FindItemRow(const FName ItemID) const
{
	const UWorld* World = GetWorld();
	// 월드 생성 이후 사용 가능, 서버 전용
	if (!World || World->GetNetMode() == ENetMode::NM_Client)
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
	
	return ItemTable->FindRow<FERNItemTable>(ItemID, TEXT("FindItemRow"));
}

const UItemDataAssetBase* UItemManagerSubsystem::LoadItemDataAssetSync(const FName ItemID)
{
	const UWorld* World = GetWorld();
	// 월드 생성 이후 사용 가능, 서버 전용
	if (!World || World->GetNetMode() == ENetMode::NM_Client)
	{
		return nullptr;
	}
	
	// 요청받은 아이템 키가 캐싱되어 있다면 캐시 데이터 반환
	if (const TObjectPtr<const UItemDataAssetBase>* Cached = ItemDataAssetCache.Find(ItemID))
	{
		return Cached->Get();
	}
	
	const FERNItemTable* Row = FindItemRow(ItemID);
	// ItemID에 해당하는 테이블 행이 없거나 경로가 존재하지 않는 경우
	if (!Row || Row->DataAsset.IsNull())
	{
		return nullptr;
	}
	
	// 동기 로드
	const UItemDataAssetBase* LoadedDataAsset = Cast<UItemDataAssetBase>(
		UAssetManager::GetStreamableManager().LoadSynchronous(
			Row->DataAsset.ToSoftObjectPath(), false
		)
	);
	if (!LoadedDataAsset)
	{
		return nullptr;
	}
	
	// 캐싱 후 반환
	ItemDataAssetCache.Add(ItemID, LoadedDataAsset);
	return LoadedDataAsset;
}

void UItemManagerSubsystem::PreloadItemDataAssetAsync(const FName ItemID)
{
	const UWorld* World = GetWorld();
	// 월드 생성 이후 사용 가능, 서버 전용
	if (!World || World->GetNetMode() == ENetMode::NM_Client)
	{
		return;
	}
	
	// 요청받은 아이템 키가 캐싱되어 있거나 이미 비동기 로드 중일 경우
	if (ItemDataAssetCache.Contains(ItemID) || PendingItemDataAssetLoads.Contains(ItemID))
	{
		return;
	}
	
	const FERNItemTable* Row = FindItemRow(ItemID);
	// ItemID에 해당하는 테이블 행이 없거나 경로가 존재하지 않는 경우
	if (!Row || Row->DataAsset.IsNull())
	{
		return;
	}
	
	const TSoftObjectPtr<const UItemDataAssetBase> DataAssetRef = Row->DataAsset;
	// 비동기 로드가 나중에 실행될 수 있기 때문에 약한 참조
	TWeakObjectPtr<UItemManagerSubsystem> WeakThis(this);
	
	// 비동기 로드
	const TSharedPtr<FStreamableHandle> Handle = UAssetManager::GetStreamableManager().RequestAsyncLoad(
		DataAssetRef.ToSoftObjectPath(),
		FStreamableDelegate::CreateLambda([WeakThis, ItemID, DataAssetRef]()
		{
			if (!WeakThis.IsValid())
			{
				return;
			}

			if (const UItemDataAssetBase* LoadedDataAsset = DataAssetRef.Get())
			{
				WeakThis->ItemDataAssetCache.Add(ItemID, LoadedDataAsset);
			}
			
			// 비동기 로드가 완료된 후 추적 테이블에서 해당 요청 객체 제거
			WeakThis->PendingItemDataAssetLoads.Remove(ItemID);
		})
	);
	
	// 비동기 로드 요청 추적 테이블에 해당 요청 객체 추가
	PendingItemDataAssetLoads.Add(ItemID, Handle);
}
