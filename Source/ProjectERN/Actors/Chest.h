// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/IInteractable.h"
#include "Chest.generated.h"

class UNiagaraSystem;
class UItemManagerSubsystem;
class USphereComponent;

UCLASS()
class PROJECTERN_API AChest : public AActor, public IInteractable
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AChest();
	
	// IInteractable
	virtual void Interact_Implementation(APlayerController* PlayerController) override;
	virtual bool CanInteract_Implementation() const override;
	virtual FText GetInteractionText_Implementation() const override;
	virtual EInteractionExecutionPolicy GetInteractionExecutionPolicy_Implementation() const override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
private:
    // Get ItemManager
	UItemManagerSubsystem* GetItemManager() const;
	
	// Start Dissolve
	UFUNCTION(NetMulticast, Reliable)
	void Dissolve();
	
	// Dissolve Animation
	void UpdateDissolve();
	
	// Collision Bind Handler
	// Begin Overlap
	UFUNCTION()
	void OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	// End Overlap
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
	
	// DropTable
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess="true"))
	TObjectPtr<UDataTable> DropTable;
	
	// VFX
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess="true"))
	TObjectPtr<UNiagaraSystem> InteractEffect;
	
	// SFX
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess="true"))
	TObjectPtr<USoundBase> InteractSound;
	
	// 중복 방지 플래그
	bool bOpened = false;
	
	// 제거 플래그
	bool bDestroyed = false;
	
	// StaticMesh DynamicMaterialInstance
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> DynamicMaterial0;
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> DynamicMaterial1;
	
	// Dissolve Timer
	FTimerHandle DissolveTimerHandle;
	// Dissolve Animation Time
	const float DissolveTime = 1.0f;
	// Dissolve Timer Rate
	const float DissolveRate = 0.1f;
	// Remaining Dissolve Animation Time
	float RemainingDissolveTime = 0.0f;
	
};
