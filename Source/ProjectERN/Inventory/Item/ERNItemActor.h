// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Data/ERNItemRuntimeState.h"
#include "Data/ERNItemTable.h"
#include "GameFramework/Actor.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Interfaces/IInteractable.h"
#include "ERNItemActor.generated.h"

class UProjectileMovementComponent;
class UNiagaraSystem;
class UWidgetComponent;
class UNiagaraComponent;
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
	virtual void ActivateInteract_Implementation() const override;
	virtual void EndInteract_Implementation(APlayerController* PlayerController) override;
	virtual FText GetInteractionText_Implementation() const override;
	virtual EInteractionExecutionPolicy GetInteractionExecutionPolicy_Implementation() const override;
	
	// Item Launch
	void Launch(const FVector& Direction) const;
	// Item RuntimeState Initialization
	UFUNCTION(BlueprintCallable)
	void InitializeRuntimeState(const FItemRuntimeState& InItemRuntimeState);
	// Item DataAsset 적용 진입점
	UFUNCTION(BlueprintCallable)
	void ApplyItemDataAsset(const UItemDataAssetBase* DataAsset);
	// Check Owner PlayerController
	bool CanBeInteractedBy(const APlayerController* PC) const;
	// 현재 클라만 보이도록 설정
	void UpdateOwnerOnlyVisibility() const;
	
protected:
	virtual void BeginPlay() override;
	
private:
	// Get ItemManager
	UItemManagerSubsystem* GetItemManager() const;
	
	// Set Mesh
	void SetMesh(const UItemDataAssetBase* DA) const;
	
	// Set Effect
	void SetEffect() const;
	
	// Set Sound
	void SetSound(const UItemDataAssetBase* DA);
	
	// ItemRuntimeState 기반 DataAsset 비동기 로드 함수
	void LoadItemDataAssetFromRuntimeStateAsync();
	
	// ItemRuntimeState Replicate Handle
	UFUNCTION()
	void OnRep_ItemRuntimeState();
	
private:
	// Collision
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess="true"))
	TObjectPtr<USphereComponent> Collision;
	
	// Mesh
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess="true"))
	TObjectPtr<UStaticMeshComponent> StaticMesh;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess="true"))
	TObjectPtr<USkeletalMeshComponent> SkeletalMesh;
	
	// InteractionCollision
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess="true"))
	TObjectPtr<USphereComponent> InteractionCollision; 
	
	// MovementComponent
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess="true"))
	TObjectPtr<UProjectileMovementComponent> MovementComponent;
	
	// Effect
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess="true"))
	TObjectPtr<UNiagaraComponent> EffectComponent;
	
	// Prompt
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess="true"))
	TObjectPtr<UWidgetComponent> PromptComponent;
	
	// VFX
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(AllowPrivateAccess="true"))
	TObjectPtr<UNiagaraSystem> CommonEffect;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(AllowPrivateAccess="true"))
	TObjectPtr<UNiagaraSystem> UncommonEffect;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(AllowPrivateAccess="true"))
	TObjectPtr<UNiagaraSystem> RareEffect;
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(AllowPrivateAccess="true"))
	TObjectPtr<UNiagaraSystem> LegendaryEffect;
	
	// SFX
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(AllowPrivateAccess="true"))
	TObjectPtr<USoundBase> PickupSound;
	
	// Item Runtime State
	UPROPERTY(ReplicatedUsing="OnRep_ItemRuntimeState", VisibleInstanceOnly, BlueprintReadWrite, meta=(AllowPrivateAccess="true"))
	FItemRuntimeState ItemRuntimeState;
	
};
