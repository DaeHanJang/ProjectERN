// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Character/Player/ProjectERNCharacter.h"
#include "GameFramework/Actor.h"
#include "Interfaces/IInteractable.h"
#include "ERNNightLordGrace.generated.h"

class UNiagaraComponent;
class USphereComponent;
class UWidgetComponent;

UCLASS()
class PROJECTERN_API AERNNightLordGrace : public AActor, public IInteractable
{
	GENERATED_BODY()
	
public:	
	AERNNightLordGrace();
	
	// IInteractable
	virtual void Interact_Implementation(APlayerController* PlayerController) override;
	virtual bool CanInteract_Implementation() const override;
	virtual void ActivateInteract_Implementation() const override;
	virtual void EndInteract_Implementation(APlayerController* PlayerController) override;
	virtual FText GetInteractionText_Implementation() const override;
	virtual EInteractionExecutionPolicy GetInteractionExecutionPolicy_Implementation() const override;

protected:
	virtual void BeginPlay() override;
	
private:
	// PlayerCharacter Attributes Restore
	void RestoreAttributes(const AProjectERNCharacter* TargetCharacter) const;
	
	// Collision BeginOverlap Handler
	UFUNCTION()
	void OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, 
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
	
private:	
	// Collision
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Grace|Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<USphereComponent> InteractionComponent;
	
	// Mesh
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Grace|Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UStaticMeshComponent> GraceMesh;
	
	// Effect
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Grace|Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UNiagaraComponent> EffectComponent;
	
	// Prompt
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Grace|Components", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UWidgetComponent> InteractionPromptWidget;
	
	// LevelUp Widget
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Grace|Widget", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UUserWidget> LevelUpPopupWidget;
	
	// 델리게이트에 바인딩할 함수
	UFUNCTION()
	void HandlePopupClosed();
	
};
