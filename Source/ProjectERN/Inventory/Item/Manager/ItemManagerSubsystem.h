// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/StreamableManager.h"
#include "Inventory/Item/Data/ERNItemTable.h"
#include "Inventory/Item/Data/ERNItemAssetLoadTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ItemManagerSubsystem.generated.h"

class UConsumableItemDataAsset;
class UEquipableItemDataAsset;

// 정책 플래그에 따른 비동기 로드 요청 추적 구조체
struct FPendingItemDataAssetLoad
{
	// Item 로드 정책 플래그
	EItemAssetLoadFlags RequestedLoadFlags = EItemAssetLoadFlags::None;
	// AssetManager에서 보낸 비동기 로드 요청 핸들
	TSharedPtr<FStreamableHandle> Handle;
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
	
	// TODO: Spawn Item
	
	// 아이템 검증
	bool ItemValid(const FName ItemID) const;
	
	// 아이템 데이터 애셋 동기 로드
	const UItemDataAssetBase* LoadItemDataAssetSync(const FName ItemID, const EItemAssetLoadFlags LoadFlags);
	// 아이템 데이터 애셋 비동기 로드
	void PreloadItemDataAssetAsync(const FName ItemID, const EItemAssetLoadFlags LoadFlags);
	
private:
	// 서버 검증
	bool CanAccessItemTable() const;
	// 데이터 테이블에서 아이템 키에 해당하는 행 가져오기
	const FERNItemTable* FindItemRow(const FName ItemID) const;
	
private:
	// 아이템 테이블 (핵심 데이터이기 때문에 TObjectPtr로 강하게 참조)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Data", meta=(AllowPrivateAccess="true"))
	TObjectPtr<const UDataTable> ItemTable = nullptr;
	
	// 검증된 아이템 키에 해당하는 고유 데이터를 캐싱한 맵
	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<const UItemDataAssetBase>> ItemDataAssetCache;
	
	// 비동기 로드 요청 추적 테이블 (중복 요청 방지, 비동기 로드 요청 객체 보관, 로드 완료 시점 관리)
	TMap<FName, FPendingItemDataAssetLoad> PendingItemDataAssetLoads;
	
};
