// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "ERNInputConfig.generated.h"

class UInputMappingContext;
class UInputAction;

USTRUCT(BlueprintType)
struct FERNInputAction
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	TObjectPtr<const UInputAction> InputAction = nullptr;

	UPROPERTY(EditDefaultsOnly, Meta=(Categories="Input"))
	FGameplayTag InputTag;
};

UCLASS()
class PROJECTERN_API UERNInputConfig : public UDataAsset
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta=(TitleProperty="InputTag"))
	TArray<FERNInputAction> NativeInputActions;

	// UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	// TObjectPtr<UInputMappingContext> DefaultMappingContext;

	const UInputAction* FindNativeInputActionByTag(const FGameplayTag& InputTag) const;
};
