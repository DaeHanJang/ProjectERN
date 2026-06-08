// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ERNInteractableWidget.h"
#include "DHRollWidget.generated.h"

class UTextBlock;
class UButton;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRollClicked, APlayerController*, PlayerController);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRewardClicked, APlayerController*, PlayerController);

/**
 * 
 */
UCLASS()
class PROJECTERN_API UDHRollWidget : public UERNInteractableWidget
{
	GENERATED_BODY()
	
public:
	UDHRollWidget(const FObjectInitializer& ObjectInitializer);
	
	void SetText(const int32 Grade) const;
	void SetVisibilityRollButton(const bool bVisibility) const;
	void SetVisibilityRewardButton(const bool bVisibility) const;
	
protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	
private:
	UFUNCTION()
	void OnRoll();
	
	UFUNCTION()
	void OnReward();
	
public:
	FOnRollClicked OnRollClicked;
	FOnRewardClicked OnRewardClicked;
	
private:
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> TextBlock;
	
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> RollButton;
	
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> RewardButton;
	
};
