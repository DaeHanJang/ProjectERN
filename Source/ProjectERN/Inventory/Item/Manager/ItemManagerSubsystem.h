// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/StreamableManager.h"
#include "Inventory/Item/Data/ERNDropTable.h"
#include "Inventory/Item/Data/ERNItemTable.h"
#include "Inventory/Item/Data/ERNItemAssetLoadTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ItemManagerSubsystem.generated.h"

struct FItemRuntimeState;
class UConsumableItemDataAsset;
class UEquipableItemDataAsset;

DECLARE_DELEGATE_OneParam(FOnItemDataAssetLoaded, const UItemDataAssetBase*);

// 정책 플래그에 따른 비동기 로드 요청 추적 구조체
struct FPendingItemDataAssetLoad
{
	// Item 로드 정책 플래그
	EItemAssetLoadFlags RequestedLoadFlags = EItemAssetLoadFlags::None;
	// AssetManager에서 보낸 비동기 로드 요청 핸들
	TSharedPtr<FStreamableHandle> Handle;
	
	TArray<FOnItemDataAssetLoaded> OnLoadedCallbacks;
};

/**
 * 
 */
UCLASS()
class PROJECTERN_API UItemManagerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	
	// ItemTable에서 ItemID에 해당하는 RowData 가져오기 
	const FERNItemTable* FindItemRow(const FName ItemID) const;
	// 아이템 검증
	UFUNCTION(BlueprintPure, Category="ItemManager")
	bool ItemValid(const FName ItemID) const;
	// 아이템 생성
	UFUNCTION(BlueprintCallable, Category="ItemManager")
	AActor* SpawnItem(const FItemRuntimeState& ItemRuntimeState, const FVector& Location, const FRotator& Rotation);
	// 드롭 테이블 기반 아이템 데이터 생성
	UFUNCTION(BlueprintCallable, Category="ItemManager")
	bool RollItemFromDropTable(const UDataTable* DropTable, FItemRuntimeState& OutItemRuntimeState, EDropItemType DropItemType = EDropItemType::None) const;

	// 아이템 데이터 애셋 동기 로드
	UFUNCTION(BlueprintCallable, Category="ItemManager")
	const UItemDataAssetBase* LoadItemDataAssetSync(const FName ItemID, const EItemAssetLoadFlags LoadFlags);
	// 아이템 데이터 애셋 비동기 로드
	void PreloadItemDataAssetAsync(const FName ItemID, const EItemAssetLoadFlags LoadFlags, FOnItemDataAssetLoaded OnLoaded = FOnItemDataAssetLoaded());
		
private:
	// 아이템 테이블 (핵심 데이터이기 때문에 TObjectPtr로 강하게 참조)
	UPROPERTY(Transient)
	TObjectPtr<UDataTable> ItemTable = nullptr;
	
	// 검증된 아이템 키에 해당하는 고유 데이터를 캐싱한 맵
	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<UItemDataAssetBase>> ItemDataAssetCache;
	
	// 비동기 로드 요청 추적 테이블 (중복 요청 방지, 비동기 로드 요청 객체 보관, 로드 완료 시점 관리)
	TMap<FName, FPendingItemDataAssetLoad> PendingItemDataAssetLoads;
	
};
