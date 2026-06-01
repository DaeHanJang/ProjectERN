// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/IInteractable.h"
#include "Chest.generated.h"

class UBoxComponent;
class UWidgetComponent;
class UNiagaraComponent;
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
	virtual void EndInteract_Implementation(APlayerController* PlayerController) override;
	virtual bool CanInteract_Implementation() const override;
	virtual void ActivateInteract_Implementation() const override;
	virtual FText GetInteractionText_Implementation() const override;
	virtual EInteractionExecutionPolicy GetInteractionExecutionPolicy_Implementation() const override;
	
private:
    // Get ItemManager
	UItemManagerSubsystem* GetItemManager() const;
	
	// Start Dissolve
	UFUNCTION(NetMulticast, Reliable)
	void Dissolve();
	
	// Dissolve Animation
	void UpdateDissolve();
	
private:
	// Collision
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess="true"))
	TObjectPtr<UBoxComponent> Collision;
	
	// Mesh
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess="true"))
	TObjectPtr<USkeletalMeshComponent> SkeletalMesh;
	
	// Interaction Collision
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess="true"))
	TObjectPtr<USphereComponent> InteractionCollision;
	
	// Effect
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess="true"))
	TObjectPtr<UNiagaraComponent> EffectComponent;
	
	// Prompt
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess="true"))
	TObjectPtr<UWidgetComponent> PromptComponent;
	
	// DropTable
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess="true"))
	TObjectPtr<UDataTable> DropTable;
	
	// Animation
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess="true"))
	TObjectPtr<UAnimationAsset> InteractAnimation;
	
	// VFX
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess="true"))
	TObjectPtr<UNiagaraSystem> InteractEffect;
	
	// SFX
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess="true"))
	TObjectPtr<USoundBase> InteractSound0;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess="true"))
	TObjectPtr<USoundBase> InteractSound1;
	
	// 중복 방지 플래그
	bool bOpened = false;
	
	// 제거 플래그
	bool bDestroyed = false;
	
	// Mesh DynamicMaterialInstance
	UPROPERTY()
	TObjectPtr<UMaterialInstanceDynamic> DynamicMaterial0;
	
	// Dissolve Timer
	FTimerHandle DissolveTimerHandle;
	// Dissolve Animation Time
	const float DissolveTime = 1.0f;
	// Dissolve Timer Rate
	const float DissolveRate = 0.1f;
	// Remaining Dissolve Animation Time
	float RemainingDissolveTime = 0.0f;
	
};
