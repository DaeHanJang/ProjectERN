// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ERNMinimapTargetPoint.h"
#include "ERNMinimapPinPoint.generated.h"

class APlayerState;

UCLASS()
class PROJECTERN_API AERNMinimapPinPoint : public AERNMinimapTargetPoint
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	AERNMinimapPinPoint();
	
	void InitializePin(EERNMinimapIconType InIconType, APlayerState* InOwnerPlayerState);
	
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	
	APlayerState* GetOwnerPlayerState() const { return OwnerPlayerState.Get(); }
	
private:
	
	UPROPERTY(Replicated)
	TObjectPtr<APlayerState> OwnerPlayerState;
};
