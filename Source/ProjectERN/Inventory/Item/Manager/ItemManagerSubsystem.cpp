// Fill out your copyright notice in the Description page of Project Settings.

#include "Inventory/Item/Manager/ItemManagerSubsystem.h"

#include "ItemManagerSettings.h"
#include "Engine/AssetManager.h"
#include "Inventory/Item/ERNItemActor.h"
#include "Inventory/Item/Data/EquipableItemDataAsset.h"
#include "Inventory/Item/Data/ERNDropTable.h"
#include "Inventory/Item/Data/ItemDataAssetBase.h"

void UItemManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	// ProjectSettings에 적용되어 있는 ItemTable 적재
	if (const UItemManagerSettings* Settings = GetDefault<UItemManagerSettings>())
	{
		ItemTable = Settings->ItemTable.LoadSynchronous();
	}
		
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

const FERNItemTable* UItemManagerSubsystem::FindItemRow(const FName ItemID) const
{
	// ItemTable과 ItemID가 유효하지 않을 떄
	if (!ItemTable || ItemID.IsNone())
	{
		return nullptr;
	}
	
	return ItemTable->FindRow<FERNItemTable>(ItemID, TEXT("UItemManagerSubsystem::FindItemRow"));
}

TArray<FName> UItemManagerSubsystem::GetAllItemIDs() const
{
	if (!ItemTable)
	{
		return TArray<FName>();
	}

	return ItemTable->GetRowNames();
}

bool UItemManagerSubsystem::ItemValid(const FName ItemID) const
{
	return FindItemRow(ItemID) != nullptr;
}

AActor* UItemManagerSubsystem::SpawnItem(const FItemRuntimeState& ItemRuntimeState, const FVector& Location,
	const FRotator& Rotation)
{
	// World가 존재하지 않고 검증되지 않은 아이템일 때
	if (!GetWorld() || !ItemValid(ItemRuntimeState.GetItemID()))
	{
		return nullptr;
	}
	
	// ItemActor 생성
	AERNItemActor* Item = GetWorld()->SpawnActor<AERNItemActor>(AERNItemActor::StaticClass(), Location, Rotation);
	if (!Item)
	{
		return nullptr;
	}
	
	// ItemActor 초기화
	Item->InitializeRuntimeState(ItemRuntimeState);
	
	return Item;
}

bool UItemManagerSubsystem::RollItemFromDropTable(const UDataTable* DropTable, FItemRuntimeState& OutItemRuntimeState, EDropItemType DropItemType) const
{
	TArray<float> ItemChance;
	TMap<uint32, FERNDropTable> IndexToItemID;
	float SumChance = 0.0f;
	
	if (!DropTable || DropTable->GetRowStruct() != FERNDropTable::StaticStruct())
	{
		return false;
	}
	
	// 드롭 테이블을 읽어 확률값을 누적시키면 배열에 추가 후 [인덱스, 행 데이터]로 매핑
	DropTable->ForeachRow<FERNDropTable>(TEXT("DropTableContext"), 
		[this, &ItemChance, &IndexToItemID, &SumChance, &DropItemType](const FName& RowName, const FERNDropTable& Row)
		{
			if (Row.ItemID.IsNone() || !ItemValid(Row.ItemID) || Row.DropChance <= 0 || Row.MinCount <= 0 || Row.MinCount > Row.MaxCount)
			{
				return;
			}
			
			// WeaponType 매개변수가 설정되어 있을 경우 WeaponType과 같은 종류의 무기만 추출
			const FERNItemTable* ItemTable = FindItemRow(Row.ItemID);
			if (ItemTable->ItemType == EItemType::Equipable)
			{
				if (DropItemType != EDropItemType::None && Row.Type != DropItemType)
				{
					return;
				}
			}
			
			SumChance += Row.DropChance;
			ItemChance.Add(SumChance);
			IndexToItemID.Add(ItemChance.Num() - 1, Row);
		}
	);
	
	// 랜덤 값 생성
	const float Roll = FMath::FRandRange(0.0f, SumChance);
	
	// 확률 배열을 순회하면서 랜덤 값이 최초로 이하일 경우 아이템 데이터 세팅
	for (int32 i = 0; i < ItemChance.Num(); ++i)
	{
		if (Roll <= ItemChance[i])
		{
			const FERNDropTable& Row = IndexToItemID.FindRef(i);
			const int32 Quantity = FMath::RandRange(Row.MinCount, Row.MaxCount);
			
			OutItemRuntimeState.SetItemID(Row.ItemID);
			OutItemRuntimeState.SetQuantity(Quantity);
			if (Row.Type != EDropItemType::None && Row.Type != EDropItemType::Consumable)
			{
				RollItemAbility(OutItemRuntimeState);
			}
			
			break;
		}
	}
	
	return OutItemRuntimeState.IsValid();
}

void UItemManagerSubsystem::RollItemAbility(FItemRuntimeState& OutItemRuntimeState) const
{
	switch (const int32 Roll = FMath::RandRange(0, static_cast<int32>(EItemAbility::Max) - 1))
	{
	case 0:
		OutItemRuntimeState.SetItemAbility(EItemAbility::None);
		break;
	case 1:
		OutItemRuntimeState.SetItemAbility(EItemAbility::Health);
		break;
	case 2:
		OutItemRuntimeState.SetItemAbility(EItemAbility::Attack);
		break;
	case 3:
		OutItemRuntimeState.SetItemAbility(EItemAbility::HealthAndAttack);
		break;
	default:
		OutItemRuntimeState.SetItemAbility(EItemAbility::None);
	}
}

const UItemDataAssetBase* UItemManagerSubsystem::LoadItemDataAssetSync(const FName ItemID, const EItemAssetLoadFlags LoadFlags)
{
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
	if (const TObjectPtr<UItemDataAssetBase>* Cached = ItemDataAssetCache.Find(ItemID))
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

void UItemManagerSubsystem::PreloadItemDataAssetAsync(const FName ItemID, const EItemAssetLoadFlags LoadFlags, FOnItemDataAssetLoaded OnLoaded)
{
	// ItemID에 해당하는 테이블 행이 없거나 경로가 존재하지 않는 경우
	const FERNItemTable* Row = FindItemRow(ItemID);
	if (!Row || Row->DataAsset.IsNull())
	{
		return;
	}
	// 로드 정책이 유효하지 않을 경우 
	if (LoadFlags == EItemAssetLoadFlags::None)
	{
		return;
	}
	
	// 데이터 애셋에 정의된 리소스 비동기 로드 헬퍼 람다
	auto ExecuteAfterNestedAssetsLoaded = 
		[](const UItemDataAssetBase* DataAsset, EItemAssetLoadFlags RequestedFlags, TArray<FOnItemDataAssetLoaded> Callbacks)
	{
		if (!DataAsset)
		{
			return;
		}

		TArray<FSoftObjectPath> NestedPaths;
		// 데이터 애셋에 존재하는 Soft reference 리소스 경로 추출
		DataAsset->GatherSoftPaths(RequestedFlags, NestedPaths);
			
		// 나중에 실행할 콜백 묶음 생성
		auto ExecuteCallbacks = [DataAsset, Callbacks = MoveTemp(Callbacks)]() mutable
		{
			for (const FOnItemDataAssetLoaded& Callback : Callbacks)
			{
				Callback.ExecuteIfBound(DataAsset);
			}
		};
		
		if (NestedPaths.IsEmpty())
		{
			ExecuteCallbacks();
			return;
		}
		
		// 데이터 애셋에 정의된 리소스 비동기 로드
		UAssetManager::GetStreamableManager().RequestAsyncLoad(
			NestedPaths,
			FStreamableDelegate::CreateLambda(MoveTemp(ExecuteCallbacks))
		);
	};
	
	// 요청받은 아이템 키가 캐싱되어 있다면 데이터 애셋에 정의된 리소스 비동기 로드
	if (const TObjectPtr<UItemDataAssetBase>* Cached = ItemDataAssetCache.Find(ItemID))
	{
		if (const UItemDataAssetBase* CachedDataAsset = Cached->Get())
		{
			TArray<FOnItemDataAssetLoaded> Callbacks;
			if (OnLoaded.IsBound())
			{
				Callbacks.Add(OnLoaded);
			}
			
			// 캐시된 데이터 애셋을 사용하되 이번 요청의 로드 정책에 대한 리소스를 비동기 로드
			ExecuteAfterNestedAssetsLoaded(CachedDataAsset, LoadFlags, MoveTemp(Callbacks));
		}
		return;
	}
	
	// 비동기 로드 요청 테이블에 요청된 ItemID를 찾고 로드 정책만 누적 없으면 추가
	FPendingItemDataAssetLoad& PendingLoad = PendingItemDataAssetLoads.FindOrAdd(ItemID);
	PendingLoad.RequestedLoadFlags |= LoadFlags;
	
	if (OnLoaded.IsBound())
	{
		PendingLoad.OnLoadedCallbacks.Add(OnLoaded);
	}
	
	// 이미 DataAsset 비동기 로드가 진행 중이라면 중복 요청 방지
	if (PendingLoad.Handle.IsValid())
	{
		return;
	}
	
	const TSoftObjectPtr<UItemDataAssetBase> DataAssetRef = Row->DataAsset;
	// 비동기 로드가 나중에 실행될 수 있기 때문에 약한 참조
	TWeakObjectPtr<UItemManagerSubsystem> WeakThis(this);
	
	// 아이템 데이터 애셋 비동기 로드
	PendingLoad.Handle = UAssetManager::GetStreamableManager().RequestAsyncLoad(
		DataAssetRef.ToSoftObjectPath(),
		FStreamableDelegate::CreateLambda([WeakThis, ItemID, DataAssetRef, ExecuteAfterNestedAssetsLoaded]()
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
			TArray<FOnItemDataAssetLoaded> Callbacks = MoveTemp(PendingLoadPtr->OnLoadedCallbacks);
			
			if (PendingLoadPtr->Handle.IsValid())
			{
				// 이 비동기 요청 핸들 제거
				PendingLoadPtr->Handle->ReleaseHandle();
			}
			
			// 비동기 로드 요청 추적 테이블에서 제거
			WeakThis->PendingItemDataAssetLoads.Remove(ItemID);
			
			// 비동기 로드 완료된 데이터 애셋 가져오기
			UItemDataAssetBase* LoadedDataAsset = DataAssetRef.Get();
			if (!LoadedDataAsset)
			{
				return;
			}
			
			// 캐싱
			WeakThis->ItemDataAssetCache.Add(ItemID, LoadedDataAsset);
			
			// 데이터 에셋 안에 있는 필요한 리소슫르을 비동기 로드
			ExecuteAfterNestedAssetsLoaded(LoadedDataAsset, RequestedFlags, MoveTemp(Callbacks));
		})
	);
}
