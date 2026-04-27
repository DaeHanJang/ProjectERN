// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Data/ERNItemRuntimeState.h"
#include "Data/ERNItemTable.h"
#include "GameFramework/Actor.h"
#include "Interfaces/IInteractable.h"
#include "ERNItemActor.generated.h"

class UItemManagerSubsystem;
class USphereComponent;

UCLASS()
class PROJECTERN_API AERNItemActor : public AActor, public IInteractable
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AERNItemActor();
	
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
	// Getter
	FORCEINLINE FItemRuntimeState& GetItemRuntimeState() { return ItemRuntimeState; }
	
	// IInteractable
	virtual void Interact_Implementation(APlayerController* PlayerController) override;
	virtual bool CanInteract_Implementation() const override;
	virtual FText GetInteractionText_Implementation() const override;
	
	// Item RuntimeState Initialization
	UFUNCTION(BlueprintCallable)
	void InitRuntimeState(const FName ItemID, const int32 Quantity);
	// Item Asset Load
	UFUNCTION(BlueprintCallable)
	void ApplyLoadedData(const UItemDataAssetBase* DataAsset);

protected:
	virtual void BeginPlay() override;
	
private:
	// Get ItemManager
	UItemManagerSubsystem* GetItemManager() const;
	
	// Set Mesh
	void SetMesh(const UItemDataAssetBase* DA) const;
	
	void RefreshFromItemRuntimeState();
	
	UFUNCTION()
	void OnRep_ItemRuntimeState();
	
	UFUNCTION()
	void OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
	
private:
	// Collision
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess="true"))
	TObjectPtr<USphereComponent> Collision;
	
	// Mesh
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess="true"))
	TObjectPtr<UStaticMeshComponent> StaticMesh;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess="true"))
	TObjectPtr<USkeletalMeshComponent> SkeletalMesh;
	
	// Item Runtime State
	UPROPERTY(ReplicatedUsing="OnRep_ItemRuntimeState", EditDefaultsOnly, BlueprintReadWrite, meta=(AllowPrivateAccess="true"))
	FItemRuntimeState ItemRuntimeState;
	
};
