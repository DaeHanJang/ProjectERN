// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ERNPlayerController.generated.h"

class AChurch;
class IInteractable;
class UInputMappingContext;
class UInputAction;
class UUserWidget;
class AERNPlayerState;
class AERNDamageTextActor;
class AERNBossCharacter;
class UERNBossHealthBarWidget;
class UCameraShakeBase;

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
	
	/** Inventory input action */
	UPROPERTY(EditAnywhere, Category="Input|Actions")
	UInputAction* InventoryAction;

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

	// 인벤토리 위젯 클래스
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> InventoryWidgetClass;
	
	// 인벤토리 위젯을 숨길 맵 이름 목록 (부분 일치)
	UPROPERTY(EditAnywhere, Category = "UI")
	TArray<FString> HideInventoryWidgetMapNames;
	
	UPROPERTY(Transient)
	UUserWidget* InventoryWidget = nullptr;
	
	/** Gameplay initialization */
	virtual void BeginPlay() override;
	
	virtual void AcknowledgePossession(class APawn* P) override;

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
	
private:
	// 인벤토리 UI에 이벤트 바인딩
	void RefreshInventoryWidget() const;

public:
	// 캐릭터 선택 위젯
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> CharacterSelectionWidgetClass;

	// 상점 메인 위젯 클래스
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> ShopMainWidgetClass;
	
	// LevelUp 위젯 클래스
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> LevelUpWidgetClass;

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
	AActor* GetCurrentInteractable() const { return CurrentInteractableActor.Get(); }
	void SetCurrentInteractable(AActor* Interactable) { CurrentInteractableActor = Interactable; }
	void ClearCurrentInteractable() { CurrentInteractableActor = nullptr; }
	
	// 인벤토리 열기
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void InventoryOpen();
	
	// 인벤토리 닫기
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void InventoryClose();

	// 데미지 텍스트 액터 클래스 (BP에서 설정)
	UPROPERTY(EditDefaultsOnly, Category = "UI|DamageText")
	TSubclassOf<AERNDamageTextActor> DamageTextActorClass;

	// 데미지 텍스트 좌우 랜덤 오프셋 범위 (±값, 같은 위치 겹침 방지)
	UPROPERTY(EditDefaultsOnly, Category = "UI|DamageText", meta = (ClampMin = "0.0"))
	float DamageTextRandomOffset = 10.f;

	// 데미지 텍스트 표시 (공격자 클라이언트에서 호출)
	UFUNCTION(Client, Unreliable)
	void Client_ShowDamageText(FVector Location, float Damage);

	// 보스 체력바 위젯 클래스 (BP에서 설정)
	UPROPERTY(EditDefaultsOnly, Category = "UI|BossHealthBar")
	TSubclassOf<UERNBossHealthBarWidget> BossHealthBarWidgetClass;

	// 보스 체력바 표시 (보스 조우 시)
	UFUNCTION(Client, Reliable)
	void Client_ShowBossHealthBar(AERNBossCharacter* Boss);

	// 보스 체력 업데이트 + 데미지 누적
	UFUNCTION(Client, Unreliable)
	void Client_UpdateBossHealthBar(float HealthPercent, float MyDamageDealt);

	// 보스 체력바 숨김
	UFUNCTION(Client, Reliable)
	void Client_HideBossHealthBar();

	// 카메라 흔들림 (공격자 본인/피격자 본인 등 단일 PC 대상)
	UFUNCTION(Client, Unreliable)
	void Client_PlayCameraShake(TSubclassOf<UCameraShakeBase> ShakeClass, float Scale);

	// 화면 페이드 인 (검은화면 → 게임화면, 인트로 시퀀스용)
	UFUNCTION(Client, Reliable)
	void Client_StartFadeIn(float Duration);
	
	// 교회 상호작용 후 효과 제거
	UFUNCTION(Client, Reliable)
	void Client_CompleteChurchInteraction(AChurch* Church, FVector EffectLocation);

	// 인트로 타이틀 위젯 표시 (타이밍은 위젯 내부 UMG Animation으로 관리)
	UFUNCTION(Client, Reliable)
	void Client_ShowIntroTitleWidget();

private:
	// 보스 체력바 위젯 인스턴스
	UPROPERTY(Transient)
	UERNBossHealthBarWidget* BossHealthBarWidget = nullptr;

	// 현재 상호작용 가능한 액터
	UPROPERTY()
	TWeakObjectPtr<AActor> CurrentInteractableActor;

	// 캐릭터 타입 체크 타이머
	FTimerHandle CharacterTypeCheckTimerHandle;

	// 캐릭터 타입 체크 시작 시간
	float CharacterTypeCheckStartTime;
};
