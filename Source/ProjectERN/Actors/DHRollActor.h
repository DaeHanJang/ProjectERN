// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/IInteractable.h"
#include "DHRollActor.generated.h"

class UWidgetComponent;
class USphereComponent;

UCLASS()
class PROJECTERN_API ADHRollActor : public AActor, public IInteractable
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ADHRollActor();
	
	virtual EInteractionExecutionPolicy GetInteractionExecutionPolicy_Implementation() const override;
	virtual void ActivateInteract_Implementation() const override;
	virtual bool CanInteract_Implementation() const override;
	virtual void Interact_Implementation(APlayerController* PlayerController) override;
	virtual void EndInteract_Implementation(APlayerController* PlayerController) override;
	
	void Init();
	
	UFUNCTION()
	void Roll(APlayerController* PlayerController);
	
	UFUNCTION()
	void Reward(APlayerController* PlayerController);
	
	UFUNCTION()
	void Close();
	
private:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Collision", meta=(AllowPrivateAccess=true))
	TObjectPtr<USphereComponent> Collision;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Mesh", meta=(AllowPrivateAccess=true))
	TObjectPtr<UStaticMeshComponent> Mesh;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Prompt", meta=(AllowPrivateAccess=true))
	TObjectPtr<UWidgetComponent> Prompt;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Prompt", meta=(AllowPrivateAccess=true))
	TObjectPtr<UUserWidget> RollWidget;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="UI", meta=(AllowPrivateAccess=true))
	TSubclassOf<UUserWidget> RollWidgetClass;
	
	UPROPERTY(Transient)
	TMap<APlayerController*, int32> PlayerControllersRewordGrade;
	
	UPROPERTY(Transient)
	TMap<APlayerController*, int32> PlayerControllersOverFlag;
	
	UPROPERTY(Transient)
	TSet<APlayerController*> PlayerControllersRewordFlag;

};
