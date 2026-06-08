// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ItemChestSpawner.generated.h"

class AChest;

UCLASS()
class PROJECTERN_API AItemChestSpawner : public AActor
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AItemChestSpawner();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
public:
	// 에디터 전용 GUID 방식 ID 생성 함수
	UFUNCTION(BlueprintCallable, CallInEditor)
	void EnsureItemChestSpawnerID();

public:
	void SpawnChest();
	void HandleChestActivated();
	
private:
	void ClearSpawnedChest();
	
private:
	// ERNWorldManagerSubsystem에 등록될 아이디
	UPROPERTY(EditAnywhere)
	FName ItemChestSpawnerID;
	
	UPROPERTY(EditAnywhere)
	TSubclassOf<AChest> ChestClass;
	
	// 상자에서 사용할 드롭 테이블
	UPROPERTY(EditAnywhere, meta=(AllowPrivateAccess = "true"))
	TObjectPtr<UDataTable> DropTable;
	
	UPROPERTY(Transient)
	TObjectPtr<AChest> SpawnedChest;
};
