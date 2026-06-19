// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/IInteractable.h"
#include "ERNAccountStatActor.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UWidgetComponent;

/**
 * AERNAccountStatActor - 로비에 설치하는 계정 영구 스탯 투자 액터.
 * AERNUpgradeActor와 동일한 IInteractable 패턴. 상호작용 시 계정 스탯 위젯을 띄운다.
 * 투자/초기화는 GameInstance(로컬 세이브)에서 처리하고, 위젯이 pawn->RefreshAccountBuffs()로 즉시 반영.
 */
UCLASS()
class PROJECTERN_API AERNAccountStatActor : public AActor, public IInteractable
{
	GENERATED_BODY()

public:
	AERNAccountStatActor();

	virtual void BeginPlay() override;

	// ===== IInteractable 구현 =====
	virtual void Interact_Implementation(APlayerController* PlayerController) override;
	virtual bool CanInteract_Implementation() const override;
	virtual FText GetInteractionText_Implementation() const override;
	virtual void EndInteract_Implementation(APlayerController* PlayerController) override;
	virtual void ActivateInteract_Implementation() const override;
	virtual EInteractionExecutionPolicy GetInteractionExecutionPolicy_Implementation() const override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AccountStat")
	FText InteractionPrompt = FText::FromString(TEXT("영구 스탯 강화"));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AccountStat")
	bool bIsActive = true;

	// 에디터에서 할당할 계정 스탯 위젯 클래스 (WBP_AccountStat)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "AccountStat|UI")
	TSubclassOf<UUserWidget> AccountStatWidgetClass;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<USphereComponent> InteractionSphere;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UWidgetComponent> InteractionPromptWidget;

private:
	UPROPERTY()
	UUserWidget* ActiveWidget = nullptr;

	UFUNCTION()
	void HandleWidgetClosed();
};
