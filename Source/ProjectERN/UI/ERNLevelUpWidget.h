// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UI/ERNInteractableWidget.h"
#include "ERNLevelUpWidget.generated.h"

struct FOnAttributeChangeData;
class UTextBlock;

/**
 * 
 */
UCLASS()
class PROJECTERN_API UERNLevelUpWidget : public UERNInteractableWidget
{
	GENERATED_BODY()
	
public:
	UERNLevelUpWidget(const FObjectInitializer& ObjectInitializer);
	
	UFUNCTION(BlueprintCallable, Category="LevelUp")
	void RefreshTextByCurrentLevel();
	
protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	
private:
	void OnLevelChanged(const FOnAttributeChangeData& Data);
	
private:
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> LevelUpCostTextBlock;
	
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> CurrentLevelTextBlock;
	
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> NextLevelTextBlock;
	
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> CurrentHealthTextBlock;
	
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> NextHealthTextBlock;
	
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> CurrentManaTextBlock;
	
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> NextManaTextBlock;
	
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> CurrentStaminaTextBlock;
	
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> NextStaminaTextBlock;
	
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> CurrentStrengthTextBlock;
	
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> NextStrengthTextBlock;
	
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> CurrentDefenceTextBlock;
	
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> NextDefenceTextBlock;
	
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> CurrentStaggerTextBlock;
	
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> NextStaggerTextBlock;
	
	FDelegateHandle OnLevelChangedHandle;
	
};
