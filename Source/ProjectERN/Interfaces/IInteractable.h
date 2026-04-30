// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "IInteractable.generated.h"

// Interaction Execution Policy
UENUM(BlueprintType)
enum class EInteractionExecutionPolicy : uint8
{
	LocalOnly       UMETA(DisplayName="Local Only"),
	ServerAuthority UMETA(DisplayName="Server Authority")
};

UINTERFACE(MinimalAPI, Blueprintable)
class UInteractable : public UInterface
{
	GENERATED_BODY()
};

class IInteractable
{
	GENERATED_BODY()

public:
	// E키로 상호작용 시 호출
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	void Interact(APlayerController* PlayerController);

	// 상호작용 가능한지 여부
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	bool CanInteract() const;

	// 상호작용 UI 텍스트
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	FText GetInteractionText() const;

	// 상호작용 정책 가져오기
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaction")
	EInteractionExecutionPolicy GetInteractionExecutionPolicy() const;
	
};
