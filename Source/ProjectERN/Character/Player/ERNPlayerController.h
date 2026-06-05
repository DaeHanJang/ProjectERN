// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "Components/SlateWrapperTypes.h"
#include "ERNPlayerController.generated.h"

class AERNMinimapPinPoint;
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
class UPostProcessComponent;
class UERNCompassWidget;
class UERNSkillCoolPanel;

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

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

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
	
	// 플레이어 캐릭터 생사 확인
	bool IsPlayerAlive() const;
	
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
	
	UFUNCTION(Client, Reliable)
	void Client_ResetInteractionInputState();

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

	// === 게임 종료 화면 ===
	UPROPERTY(EditDefaultsOnly, Category = "UI|EndScreen")
	TSubclassOf<UUserWidget> VictoryBannerWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "UI|EndScreen")
	TSubclassOf<UUserWidget> DefeatBannerWidgetClass;

	// GameState Multicast가 호출 — 로컬에서 승/패 배너 위젯 생성
	void ShowEndScreen(bool bVictory);

	// 전과 위젯 "로비로" 버튼 → 서버에 복귀 준비 통지
	UFUNCTION(BlueprintCallable, Server, Reliable, Category = "UI|EndScreen")
	void Server_RequestReturnToLobby();

	// 전과 위젯 "취소" 버튼 → 서버에 복귀 신청 취소 통지
	UFUNCTION(BlueprintCallable, Server, Reliable, Category = "UI|EndScreen")
	void Server_CancelReturnToLobby();

	// 카메라 흔들림 (공격자 본인/피격자 본인 등 단일 PC 대상)
	UFUNCTION(Client, Unreliable)
	void Client_PlayCameraShake(TSubclassOf<UCameraShakeBase> ShakeClass, float Scale);

	// 화면 페이드 인 (검은화면 → 게임화면, 인트로 시퀀스용)
	UFUNCTION(Client, Reliable)
	void Client_StartFadeIn(float Duration);
	
	// 교회 상호작용 후 효과 제거
	UFUNCTION(Client, Reliable)
	void Client_CompleteChurchInteraction(AChurch* Church);

	// 인트로 타이틀 위젯 표시 (타이밍은 위젯 내부 UMG Animation으로 관리)
	UFUNCTION(Client, Reliable)
	void Client_ShowIntroTitleWidget();

	// ===== 채팅 시스템 =====

	// 채팅 위젯 클래스
	UPROPERTY(EditDefaultsOnly, Category = "UI|Chat")
	TSubclassOf<UUserWidget> ChatWidgetClass;

	// 채팅 위젯을 숨길 맵 이름 목록 (부분 일치)
	UPROPERTY(EditAnywhere, Category = "UI|Chat")
	TArray<FString> HideChatWidgetMapNames;

	// 채팅 송신자 색상 팔레트 (PlayerId % Num()로 인덱싱)
	UPROPERTY(EditDefaultsOnly, Category = "UI|Chat")
	TArray<FLinearColor> ChatColorPalette;

	UPROPERTY(BlueprintReadOnly, Transient, Category = "UI|Chat")
	UUserWidget* ChatWidget = nullptr;

	// 클라가 채팅 입력 후 호출 (위젯 OnTextCommitted에서 호출)
	UFUNCTION(BlueprintCallable, Server, Reliable, Category = "UI|Chat")
	void Server_SendChat(const FString& Message);

	// 서버가 각 클라에 채팅 전달 (송신자 색 포함)
	UFUNCTION(Client, Reliable)
	void Client_ReceiveChat(const FString& Sender, const FString& Message, FLinearColor SenderColor);

	// 채팅 수신 시 BP에서 위젯에 메시지 추가 (Sender + Message + Color)
	UFUNCTION(BlueprintImplementableEvent, Category = "UI|Chat")
	void OnReceiveChatMessage(const FString& Sender, const FString& Message, FLinearColor SenderColor);
	
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
	
	
#pragma region NightRainZone
public:
	// 서버의 자기장 포스트 프로세스 상태 업데이트
	void UpdateNightRainPostProcessState_ServerOnly(bool bShouldEnable);
	
	// 클라이언트 개별의 자기장 포스트 프로세스 적용 결정
	UFUNCTION(Client, Reliable)
	void Client_SetNightRainZonePostProcessEnabled(bool bEnabled);
	
	// 자기장 무시 (인스턴스 던전 사용 중 자기장 무시)
	void SetIgnoreNightRainZone_ServerOnly(bool bIgnore);
	// 자기장 무시 가능 여부
	bool bIsIgnoreNightRainZone()const { return bIgnoreNightRainZone_Server; };

private:
	void SetNightRainZonePostProcessEnabled_Local(bool bEnabled);
	void TickNightRainZonePostProcessBlend();
	
	UPostProcessComponent* FindNightRainPostProcessComponent() const;
	void SetNightRainPostProcessBlendWeight_Local(float BlendWeight);
	
private:
	// 서버와 로컬을 비교
	bool bServerNightRainPostProcessEnabled = false;
	bool bLocalNightRainPostProcessEnabled = false;
	
	float CurrentNightRainPostProcessBlendWeight = 0.f;
	float TargetNightRainPostProcessBlendWeight = 0.f;
	
	UPROPERTY(EditDefaultsOnly, Category = "NightRain|PostProcess", meta = (ClampMin = "0.01"))
	float NightRainPostProcessInterpSpeed = 8.f;
	
	FTimerHandle NightRainPostProcessBlendTimerHandle;
	
	// 서버에서 자기장 판정 제외 여부로 사용될 플래그
	bool bIgnoreNightRainZone_Server = false;
	
#pragma endregion
	
#pragma region Minimap
protected:
	// 미니맵 위젯 클래스
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UUserWidget> MinimapWidgetClass;
	
	UPROPERTY(EditDefaultsOnly, Category = "Minimap")
	TSubclassOf<AERNMinimapPinPoint> MinimapPinClass;
	
	// 미니맵 위젯을 숨길 맵 이름 목록 (메인 전투 맵 제외 모든곳)
	UPROPERTY(EditAnywhere, Category = "UI")
	TArray<FString> HideMinimapWidgetMapNames;
	
	//미니맵 위젯
	UPROPERTY(Transient)
	UUserWidget* MinimapWidget;

	// 나침반(방위각) 위젯 클래스
	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<UERNCompassWidget> CompassWidgetClass;

	// 나침반 위젯을 숨길 맵 이름 목록
	UPROPERTY(EditAnywhere, Category = "UI")
	TArray<FString> HideCompassWidgetMapNames;

	// 나침반 위젯
	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> CompassWidget;

#pragma region CutsceneHUD
protected:
	// 컷신 중 일괄 숨김 대상이 되는 HUD 위젯 모음 (생성 시 등록)
	UPROPERTY(Transient)
	TArray<TObjectPtr<UUserWidget>> ManagedHUDWidgets;

	// 컷신 시작 시점의 위젯별 visibility 캐시 (종료 시 복원)
	UPROPERTY(Transient)
	TMap<TObjectPtr<UUserWidget>, ESlateVisibility> CachedHUDVisibilities;

	// HUD 위젯을 컷신 숨김 대상으로 등록/해제
	void RegisterHUDWidget(UUserWidget* Widget);
	void UnregisterHUDWidget(UUserWidget* Widget);

	// 컷신 서브시스템 이벤트 바인딩/해제 (로컬 컨트롤러 전용)
	void BindCutsceneEvents();
	void UnbindCutsceneEvents();

	UFUNCTION()
	void HandleCutsceneStarted();

	UFUNCTION()
	void HandleCutsceneFinished();
#pragma endregion CutsceneHUD

	/** Minimap input action */
	UPROPERTY(EditAnywhere, Category="Input|Actions")
	UInputAction* MinimapAction;

	/** Pause input action (ESC + P) */
	UPROPERTY(EditAnywhere, Category="Input|Actions")
	UInputAction* PauseAction;

	// 일시정지 메뉴 위젯 클래스
	UPROPERTY(EditDefaultsOnly, Category = "UI|Pause")
	TSubclassOf<UUserWidget> PausedWidgetClass;

	// 일시정지 메뉴 위젯 인스턴스
	UPROPERTY(Transient)
	UUserWidget* PausedWidget = nullptr;

	// 진입 안내 위젯 인스턴스
	UPROPERTY(Transient)
	class UERNEntranceWidget* EntranceWidget = nullptr;

public:
	// 일시정지 메뉴 열기 <-> 닫기
	UFUNCTION(BlueprintCallable, Category = "UI|Pause")
	void TogglePauseMenu();

	// 일시정지 메뉴 닫기 (위젯 내부 버튼에서 호출용)
	UFUNCTION(BlueprintCallable, Category = "UI|Pause")
	void ClosePauseMenu();

	// === 지역 진입 안내 위젯 ===

	// 진입 위젯 클래스 (BP에서 지정 — UERNEntranceWidget 자식)
	UPROPERTY(EditDefaultsOnly, Category = "UI|Entrance")
	TSubclassOf<class UERNEntranceWidget> EntranceWidgetClass;

	// 진입 안내 표시 (트리거 박스가 로컬에서 호출, 텍스트 전달)
	UFUNCTION(BlueprintCallable, Category = "UI|Entrance")
	void ShowEntranceWidget(const FText& EntranceText);

protected:
	
	// 미니맵 열기 <-> 닫기 관리
	UFUNCTION(BlueprintCallable, Category = "Minimap")
	void ToggleMinimap();

	// 미니맵 열기
	UFUNCTION(BlueprintCallable, Category = "Minimap")
	void MinimapOpen();
	
	// 미니맵 닫기
	UFUNCTION(BlueprintCallable, Category = "Minimap")
	void MinimapClose();
	
public:
	// 기존 위치 핀 : 삭제, 새로운 위치 핀 : 삭제 + 새로 생성
	UFUNCTION(Server, Reliable)
	void Server_RequestCreateMinimapPin(FVector WorldLocation);
	
	UFUNCTION(Server, Reliable)
	void Server_RequestRemoveMinimapPin(AERNMinimapPinPoint* PinActor);
	
private:
	void DestroyOwnedMinimapPins_ServerOnly();
	
#pragma endregion
	
#pragma region QuickSlot
	// ===== 퀵슬롯 UI =====
protected:
	// 퀵슬롯 위젯 클래스
	UPROPERTY(EditDefaultsOnly, Category="UI|QuickSlot")
	TSubclassOf<class UERNQuickSlotWidget> QuickSlotWidgetClass;

	// 퀵슬롯 UI를 숨길 맵 이름 목록
	UPROPERTY(EditAnywhere, Category="UI|QuickSlot")
	TArray<FString> HideQuickSlotMapNames;

	// 현재 생성된 퀵슬롯 위젯
	UPROPERTY(Transient)
	TObjectPtr<class UERNQuickSlotWidget> QuickSlotWidget;

private:
	// 캐릭터 변경 시 퀵슬롯 연동 갱신
	void RefreshQuickSlotWidget() const;
#pragma endregion QuickSlot

#pragma region SkillPanel
	// ===== 스킬 쿨타임 UI =====
protected:
	// 스킬 쿨타임 패널 클래스
	UPROPERTY(EditDefaultsOnly, Category="UI|SkillCooldown")
	TSubclassOf<UERNSkillCoolPanel> SkillCoolPanelWidgetClass;

	// 스킬 쿨타임 UI를 숨길 맵 이름 목록
	UPROPERTY(EditAnywhere, Category="UI|SkillCooldown")
	TArray<FString> HideSkillCoolPanelMapNames;

	// 현재 생성된 스킬 쿨타임 패널
	UPROPERTY(Transient)
	TObjectPtr<UERNSkillCoolPanel> SkillCoolPanelWidget;
	
private:
	// 캐릭터 변경 시 캐릭터에 따라 패널 변경
	void RefreshSkillCoolPanel() const;
	
#pragma endregion SkillPanel
};
