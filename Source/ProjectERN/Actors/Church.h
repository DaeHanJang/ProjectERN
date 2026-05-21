// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/IInteractable.h"
#include "Church.generated.h"

class UNiagaraSystem;
class UWidgetComponent;
class UNiagaraComponent;
class USphereComponent;

UCLASS()
class PROJECTERN_API AChurch : public AActor, public IInteractable
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AChurch();
	
	// IInteractable
	virtual void Interact_Implementation(APlayerController* PlayerController) override;
	virtual bool CanInteract_Implementation() const override;
	virtual void ActivateInteract_Implementation() const override;
	virtual void EndInteract_Implementation(APlayerController* PlayerController) override;
	virtual EInteractionExecutionPolicy GetInteractionExecutionPolicy_Implementation() const override;
	
	// Complete Interaction
	void CompleteInteractionLocally(const FVector EffectLocation) const; 
	

private:
	// Collision
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Collision", meta=(AllowPrivateAccess="true"))
	TObjectPtr<USphereComponent> Collision;
	
	// Mesh
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mesh", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UStaticMeshComponent> Mesh;
	
	// Effect
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Effect", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UNiagaraComponent> EffectComponent;
	
	// Prompt
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Prompt", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UWidgetComponent> PromptComponent;
	
	// VFX
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="VFX", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UNiagaraSystem> InteractionEffect;
	
	// SFX
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="SFX", meta=(AllowPrivateAccess="true"))
	TObjectPtr<USoundBase> InteractionSound;
	
	// Interacted PlayerController
	UPROPERTY(Transient)
	TArray<APlayerController*> InteractedPlayerControllers;
	
};
