// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Inventory/Item/Data/ERNItemRuntimeState.h"
#include "ERNConsumableBase.generated.h"

class UConsumableItemDataAsset;
class UGameplayAbility;
class USphereComponent;

UCLASS()
class PROJECTERN_API AERNConsumableBase : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AERNConsumableBase();
	
	// Getter/Setter
	FORCEINLINE const FItemRuntimeState& GetRuntimeState() const { return RuntimeState; }
	FORCEINLINE void SetRuntimeState(const FItemRuntimeState& NewRuntimeState) { RuntimeState = NewRuntimeState;}
	
	// Initialization
	void Init(const FItemRuntimeState& InRuntimeState, const UConsumableItemDataAsset* DA);
	
protected:
	// Collision
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Collision", meta=(AllowPrivateAccess="true"))
	TObjectPtr<USphereComponent> Collision;
	
	// Mesh
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mesh", meta=(AllowPrivateAccess="ture"))
	TObjectPtr<UStaticMeshComponent> Mesh;
	
	// GA
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="GameplayAbility", meta=(AllowPrivateAccess="true"))
	TSubclassOf<UGameplayAbility> GA = nullptr;
	
	// Runtime State
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="RuntimeState", meta=(AllowPrivateAccess="true"))
	FItemRuntimeState RuntimeState;
	
};
