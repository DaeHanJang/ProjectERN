// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ERNPlayerController.generated.h"

class IInteractable;
class UInputMappingContext;
class UInputAction;
class UUserWidget;
class AERNPlayerState;

/**
 *  Basic PlayerController class for a third person game
 *  Manages input mappings
 */

UCLASS(abstract)
class AERNPlayerController : public APlayerController
{
	GENERATED_BODY()
	
protected:

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category ="Input|Input Mappings")
	TArray<UInputMappingContext*> DefaultMappingContexts;

	/** Input Mapping Contexts */
	UPROPERTY(EditAnywhere, Category="Input|Input Mappings")
	TArray<UInputMappingContext*> MobileExcludedMappingContexts;

	/** Ready toggle input action */
	UPROPERTY(EditAnywhere, Category="Input|Actions")
	UInputAction* ReadyToggleAction;

	/** Interact input action */
	UPROPERTY(EditAnywhere, Category="Input|Actions")
	UInputAction* InteractAction;

	/** Mobile controls widget to spawn */
	UPROPERTY(EditAnywhere, Category="Input|Touch Controls")
	TSubclassOf<UUserWidget> MobileControlsWidgetClass;

	/** Pointer to the mobile controls widget */
	UPROPERTY()
	TObjectPtr<UUserWidget> MobileControlsWidget;

	/** If true, the player will use UMG touch controls even if not playing on mobile platforms */
	UPROPERTY(EditAnywhere, Config, Category = "Input|Touch Controls")
	bool bForceTouchControls = false;

	// UI (통합 위젯)
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> PartyStatusContainerClass;

	// 파티 상태 위젯을 숨길 맵 이름 목록 (부분 일치)
	UPROPERTY(EditAnywhere, Category = "UI")
	TArray<FString> HidePartyWidgetMapNames;

	// 준비 완료 버튼 위젯
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> ReadyButtonWidgetClass;

	// 준비 완료 버튼을 숨길 맵 이름 목록 (부분 일치)
	UPROPERTY(EditAnywhere, Category = "UI")
	TArray<FString> HideReadyButtonMapNames;

	/** Gameplay initialization */
	virtual void BeginPlay() override;

	// UI 생성 (약간 지연)
	void CreatePartyUI();

	// 닉네임 전송 재시도 (PlayerState가 준비될 때까지)
	void TrySendNickname();

	// 캐릭터 타입 확인 및 수정 (맵 이동 후)
	void CheckAndFixCharacterType();

	/** Input mapping context setup */
	virtual void SetupInputComponent() override;

	/** Returns true if the player should use UMG touch controls */
	bool ShouldUseTouchControls() const;

public:
	// 캐릭터 선택 위젯
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> CharacterSelectionWidgetClass;

	// 상점 메인 위젯 클래스
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> ShopMainWidgetClass;
	
	// 인벤토리 위젯 클래스
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> InventoryWidgetClass;

	// 준비 상태 토글 (블루프린트에서 호출)
	UFUNCTION(BlueprintCallable, Category = "Game")
	void ToggleReady();

	// 상호작용 시도
	UFUNCTION(BlueprintCallable, Category = "Interaction")
	void TryInteract();
	// ServerSide 상호작용 시도
	UFUNCTION(Server, Reliable)
	void Server_TryInteract(AActor* InteractableActor);

	// Pedestal(상호작용가능한액터)에서 호출용
	void SetCurrentInteractable(AActor* Interactable) { CurrentInteractableActor = Interactable; }
	void ClearCurrentInteractable() { CurrentInteractableActor = nullptr; }

private:
	// 현재 상호작용 가능한 액터
	UPROPERTY()
	TWeakObjectPtr<AActor> CurrentInteractableActor;

	// 캐릭터 타입 체크 타이머
	FTimerHandle CharacterTypeCheckTimerHandle;

	// 캐릭터 타입 체크 시작 시간
	float CharacterTypeCheckStartTime;
};
