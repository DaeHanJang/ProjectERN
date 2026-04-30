// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/IInteractable.h"
#include "CharacterSelectionPedestal.generated.h"

UCLASS()
class PROJECTERN_API ACharacterSelectionPedestal : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	ACharacterSelectionPedestal();

	// 받침대 메시
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* PedestalMesh;

	// 상호작용 범위
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class USphereComponent* InteractionSphere;

	// 상호작용 위젯
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	class UWidgetComponent* InteractionPromptWidget;

	// IInteractable 구현
	virtual void Interact_Implementation(APlayerController* PlayerController) override;
	virtual bool CanInteract_Implementation() const override;
	virtual FText GetInteractionText_Implementation() const override;
	virtual EInteractionExecutionPolicy GetInteractionExecutionPolicy_Implementation() const override;

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);
	
};
