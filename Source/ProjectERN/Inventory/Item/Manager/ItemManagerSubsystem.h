// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ItemManagerSubsystem.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTERN_API UItemManagerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	
	// TODO
	// 1. Spawn Item
	// 2. Validation
	// 3. DataAsset → FRuntimeItemState 
	
private:
	// 아이템 테이블 (핵심 데이터이기 때문에 TObjectPtr로 강하게 참조)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Data", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UDataTable> ItemTable = nullptr;
	
	// TODO
	// 1. Cache TMap<ItemID(=RowName), FRuntimeItemState>
	
};
