// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Character/Player/ERNPlayerState.h"
#include "Shop/Provider/ERNShopDataProvider.h"
#include "Enhancement/Provider/ERNUpgradeDataProvider.h"
#include "DLSSLibrary.h"
#include "ERNGameInstance.generated.h"

class UERNDummyShopProvider;
class UUserWidget;
class USoundMix;
class USoundClass;
class UERNSaveSettings;

UCLASS()
class PROJECTERN_API UERNGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	UERNGameInstance();

	virtual void Init() override;
	virtual void LoadComplete(const float LoadTime, const FString& MapName) override;

protected:
	void OnPostLoadMap(UWorld* LoadedWorld);
public:

	// Steam 모드 사용 여부 (false=LAN, true=Steam) - BP 메뉴 체크박스로 토글
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Network")
	bool bUseSteam = false;

	// LAN/Steam 모드 전환 (기존 세션/검색결과 정리 + NetDriver 재구성)
	UFUNCTION(BlueprintCallable, Category = "Network")
	void SetUseSteam(bool bNewUseSteam);

	// 세션 생성 (Host)
	UFUNCTION(BlueprintCallable, Category = "Network")
	void HostSession(FString ServerName, int32 MaxPlayers = 3);

	// 세션 검색 (SearchQuery가 비어있으면 모든 세션, 있으면 해당 이름 필터링)
	UFUNCTION(BlueprintCallable, Category = "Network")
	void FindSessions(FString SearchQuery = "");

	// 세션 참가 (Join)
	UFUNCTION(BlueprintCallable, Category = "Network")
	void JoinSessionByIndex(int32 SessionIndex);

	// IP로 직접 접속
	UFUNCTION(BlueprintCallable, Category = "Network")
	void JoinByIP(FString IPAddress);

	// 현재 세션 나가기
	UFUNCTION(BlueprintCallable, Category = "Network")
	void LeaveSession();

	// 블루프린트에서 세션 목록 가져오기
	UFUNCTION(BlueprintPure, Category = "Network")
	TArray<FString> GetSessionList() const { return SessionSearchResults; }

	// 현재 플레이어 닉네임 설정
	UFUNCTION(BlueprintCallable, Category = "Network")
	void SetPlayerNickname(FString Nickname) { CurrentPlayerNickname = Nickname; }

	// 현재 플레이어 닉네임 가져오기
	UFUNCTION(BlueprintPure, Category = "Network")
	FString GetPlayerNickname() const { return CurrentPlayerNickname; }

	// 현재 플레이어 닉네임
	UPROPERTY(BlueprintReadWrite, Category = "Network")
	FString CurrentPlayerNickname;

	// 현재 플레이어 캐릭터 타입 설정
	UFUNCTION(BlueprintCallable, Category = "Network")
	void SetPlayerCharacterType(ECharacterType CharacterType) { CurrentCharacterType = CharacterType; }

	// 현재 플레이어 캐릭터 타입 가져오기
	UFUNCTION(BlueprintPure, Category = "Network")
	ECharacterType GetPlayerCharacterType() const { return CurrentCharacterType; }

	// 현재 플레이어 캐릭터 타입
	UPROPERTY(BlueprintReadWrite, Category = "Network")
	ECharacterType CurrentCharacterType;

	// ===== 상점 시스템 =====

	// 상점 Provider 반환 (인터페이스)
	UFUNCTION(BlueprintCallable, Category = "Shop")
	TScriptInterface<IERNShopDataProvider> GetShopDataProvider() const;

	// 상점 Provider UObject 반환 (캐스팅용)
	UObject* GetShopDataProviderObject() const { return ShopDataProvider; }

	// ===== 강화 시스템 =====

	// 강화 Provider 반환 (인터페이스)
	UFUNCTION(BlueprintCallable, Category = "Upgrade")
	TScriptInterface<IERNUpgradeDataProvider> GetUpgradeDataProvider() const;

	// 강화 Provider UObject 반환 (캐스팅용)
	UObject* GetUpgradeDataProviderObject() const { return UpgradeDataProvider; }

	// 로딩 위젯 클래스 반환 (서브시스템용)
	UFUNCTION(BlueprintPure, Category = "Loading")
	TSubclassOf<UUserWidget> GetLoadingWidgetClass() const { return LoadingWidgetClass; }

	// ===== 설정: 저장/로드 =====

	UFUNCTION(BlueprintCallable, Category = "Settings")
	void SaveSettings();

	UFUNCTION(BlueprintCallable, Category = "Settings")
	void LoadAndApplySettings();

	UPROPERTY(BlueprintReadOnly, Category = "Settings")
	UERNSaveSettings* CachedSettings = nullptr;

	// ===== EasyMode (호스트 전용 게임플레이 옵션) =====
	// 호스트가 옵션에서 설정. 켜지면 최종 자기장 전원 탈락 시 딱 1회 전원 부활.
	// GameInstance에 두는 이유: BossPortal travel/ServerTravel을 넘어 런 내내 유지돼야 하기 때문
	UPROPERTY(BlueprintReadWrite, Category = "Settings|EasyMode")
	bool bEasyModeEnabled = false;

	UFUNCTION(BlueprintCallable, Category = "Settings|EasyMode")
	void SetEasyModeEnabled(bool bEnabled) { bEasyModeEnabled = bEnabled; }

	UFUNCTION(BlueprintPure, Category = "Settings|EasyMode")
	bool IsEasyModeEnabled() const { return bEasyModeEnabled; }

	// 1회 부활 가드 (런 단위). 로비 진입 시 ResetEasyModeRunGuard로 초기화
	bool IsEasyModeReviveUsed() const { return bEasyModeReviveUsed; }
	void MarkEasyModeReviveUsed() { bEasyModeReviveUsed = true; }
	void ResetEasyModeRunGuard() { bEasyModeReviveUsed = false; }

private:
	// EasyMode 1회 부활 사용 여부 (런 가드, BP 노출 불필요)
	bool bEasyModeReviveUsed = false;

private:
	// 저장된 DLSS 상태(켜짐/모드)를 실제 적용 (EnableDLSS + 스크린퍼센트). 설정/해상도 적용 후 마지막에 호출
	void ApplyDLSS();

public:
	// ===== 설정: 오디오 =====

	UFUNCTION(BlueprintCallable, Category = "Settings|Audio")
	void SetMasterVolume(float Volume);
	UFUNCTION(BlueprintPure, Category = "Settings|Audio")
	float GetMasterVolume() const;

	UFUNCTION(BlueprintCallable, Category = "Settings|Audio")
	void SetMusicVolume(float Volume);
	UFUNCTION(BlueprintPure, Category = "Settings|Audio")
	float GetMusicVolume() const;

	UFUNCTION(BlueprintCallable, Category = "Settings|Audio")
	void SetSFXVolume(float Volume);
	UFUNCTION(BlueprintPure, Category = "Settings|Audio")
	float GetSFXVolume() const;

	// ===== 설정: 비디오 =====

	// 0=Fullscreen, 1=WindowedFullscreen, 2=Windowed
	UFUNCTION(BlueprintCallable, Category = "Settings|Video")
	void SetWindowMode(int32 Mode);
	UFUNCTION(BlueprintPure, Category = "Settings|Video")
	int32 GetWindowMode() const;

	// 데스크탑 해상도 이하의 16:9 해상도 목록
	UFUNCTION(BlueprintPure, Category = "Settings|Video")
	TArray<FIntPoint> GetSupportedResolutions() const;

	UFUNCTION(BlueprintCallable, Category = "Settings|Video")
	void SetResolution(FIntPoint Resolution);
	UFUNCTION(BlueprintPure, Category = "Settings|Video")
	FIntPoint GetCurrentResolution() const;

	// "1920 x 1080" 형식 문자열
	UFUNCTION(BlueprintPure, Category = "Settings|Video")
	static FString FormatResolution(FIntPoint Resolution);

	// 밝기 0~100 (감마 적용 + 저장)
	UFUNCTION(BlueprintCallable, Category = "Settings|Video")
	void SetBrightness(float Value);
	UFUNCTION(BlueprintPure, Category = "Settings|Video")
	float GetBrightness() const;

	UFUNCTION(BlueprintCallable, Category = "Settings|Video")
	void SetOverallQuality(int32 Level);
	UFUNCTION(BlueprintPure, Category = "Settings|Video")
	int32 GetOverallQuality() const;

	UFUNCTION(BlueprintCallable, Category = "Settings|Video")
	void SetShadowQuality(int32 Level);
	UFUNCTION(BlueprintPure, Category = "Settings|Video")
	int32 GetShadowQuality() const;

	UFUNCTION(BlueprintCallable, Category = "Settings|Video")
	void SetAntiAliasingQuality(int32 Level);
	UFUNCTION(BlueprintPure, Category = "Settings|Video")
	int32 GetAntiAliasingQuality() const;

	UFUNCTION(BlueprintCallable, Category = "Settings|Video")
	void SetTextureQuality(int32 Level);
	UFUNCTION(BlueprintPure, Category = "Settings|Video")
	int32 GetTextureQuality() const;

	UFUNCTION(BlueprintCallable, Category = "Settings|Video")
	void SetViewDistanceQuality(int32 Level);
	UFUNCTION(BlueprintPure, Category = "Settings|Video")
	int32 GetViewDistanceQuality() const;

	UFUNCTION(BlueprintCallable, Category = "Settings|Video")
	void SetEffectsQuality(int32 Level);
	UFUNCTION(BlueprintPure, Category = "Settings|Video")
	int32 GetEffectsQuality() const;

	// 설정 적용 + 저장 (GameUserSettings.ini)
	UFUNCTION(BlueprintCallable, Category = "Settings|Video")
	void ApplyVideoSettings();

	// ===== DLSS =====
	// RTX+드라이버 지원 여부 (false면 위젯에서 DLSS 섹션 숨김)
	UFUNCTION(BlueprintPure, Category = "Settings|DLSS")
	bool IsDLSSAvailable() const;

	// 체크박스: DLSS 켜기/끄기
	UFUNCTION(BlueprintCallable, Category = "Settings|DLSS")
	void SetDLSSEnabled(bool bEnabled);
	UFUNCTION(BlueprintPure, Category = "Settings|DLSS")
	bool IsDLSSEnabled() const;

	// 드롭다운: GPU+해상도 지원 모드만 (Off/Auto 제외)
	UFUNCTION(BlueprintPure, Category = "Settings|DLSS")
	TArray<UDLSSMode> GetSupportedDLSSModes() const;

	// 품질 모드 설정/조회
	UFUNCTION(BlueprintCallable, Category = "Settings|DLSS")
	void SetDLSSMode(UDLSSMode Mode);
	UFUNCTION(BlueprintPure, Category = "Settings|DLSS")
	UDLSSMode GetDLSSMode() const;

protected:
	// 세션 인터페이스
	IOnlineSessionPtr SessionInterface;

	// 세션 검색 결과
	TSharedPtr<FOnlineSessionSearch> SessionSearch;

	// 블루프린트용 검색 결과
	UPROPERTY(BlueprintReadOnly, Category = "Network")
	TArray<FString> SessionSearchResults;

	// 방 이름만 저장 (SessionSearchResults와 인덱스 동기화)
	UPROPERTY(BlueprintReadOnly, Category = "Network")
	TArray<FString> SessionNames;

	// 검색어 저장
	FString CurrentSearchQuery;

	// 필터링된 세션의 원본 인덱스 매핑 (UI 인덱스 -> 원본 SearchResults 인덱스)
	TArray<int32> FilteredSessionIndices;

	// 세션 생성 완료 콜백
	void OnCreateSessionComplete(FName SessionName, bool bWasSuccessful);

	// 세션 검색 완료 콜백
	void OnFindSessionsComplete(bool bWasSuccessful);

	// 세션 참가 완료 콜백
	void OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);

	// 세션 파괴 완료 콜백
	void OnDestroySessionComplete(FName SessionName, bool bWasSuccessful);

	// Steam Overlay에서 "Join Game" 클릭 시 호출 (친구 초대 수락)
	void OnSessionUserInviteAccepted(
		const bool bWasSuccessful,
		const int32 ControllerId,
		FUniqueNetIdPtr UserId,
		const FOnlineSessionSearchResult& InviteResult);

	// Delegate Handles
	FDelegateHandle OnCreateSessionCompleteDelegateHandle;
	FDelegateHandle OnFindSessionsCompleteDelegateHandle;
	FDelegateHandle OnJoinSessionCompleteDelegateHandle;
	FDelegateHandle OnDestroySessionCompleteDelegateHandle;
	FDelegateHandle OnSessionUserInviteAcceptedDelegateHandle;

	// 생성할 세션 정보 임시 저장
	FString PendingServerName;
	int32 PendingMaxPlayers;

	// JoinSessionByIndex가 기존 세션 정리 후 재시도할 때 사용
	bool bHasPendingJoin = false;
	int32 PendingJoinIndex = -1;

	// bUseSteam 값에 맞게 GameNetDriver 정의를 런타임에 swap
	void ConfigureNetDriverForMode();

	// bUseSteam 값에 맞는 OSS의 SessionInterface로 재취득 + 델리게이트 재바인딩
	void RebindSessionInterface();

	// ===== 상점 시스템 =====

	/** 상점 데이터 Provider (UObject, 맵 전환에도 유지) */
	UPROPERTY()
	UObject* ShopDataProvider = nullptr;

	/** 블루프린트에서 지정할 DataTable Provider 클래스 */
	UPROPERTY(EditDefaultsOnly, Category = "Shop")
	TSubclassOf<class UERNDataTableShopProvider> DataTableProviderClass;

	// ===== 강화 시스템 =====

	/** 강화 데이터 Provider (UObject) */
	UPROPERTY()
	UObject* UpgradeDataProvider = nullptr;

	/** 블루프린트에서 지정할 강화 Provider 클래스 */
	UPROPERTY(EditDefaultsOnly, Category = "Upgrade")
	TSubclassOf<class UERNUpgradeProvider> UpgradeProviderClass;

	// 강화 시스템 초기화
	void InitializeUpgradeSystem();

	// ===== 설정: 오디오 에셋 (에디터에서 할당) =====

	UPROPERTY(EditDefaultsOnly, Category = "Settings|Audio")
	USoundMix* MasterSoundMix;

	UPROPERTY(EditDefaultsOnly, Category = "Settings|Audio")
	USoundClass* MasterSoundClass;

	UPROPERTY(EditDefaultsOnly, Category = "Settings|Audio")
	USoundClass* MusicSoundClass;

	UPROPERTY(EditDefaultsOnly, Category = "Settings|Audio")
	USoundClass* SFXSoundClass;

	// SoundClass 볼륨 적용 (MasterSoundMix override)
	void ApplyAudioVolume(USoundClass* SoundClass, float Volume);

	// ===== 로딩 화면 =====

	// 로딩 화면 위젯 클래스 (서브시스템에서 사용)
	UPROPERTY(EditDefaultsOnly, Category = "Loading")
	TSubclassOf<UUserWidget> LoadingWidgetClass;

	// ===== 인트로 시퀀스 =====

	// 인트로용 새 액터 클래스 (BP_IntroBird)
	UPROPERTY(EditDefaultsOnly, Category = "Intro")
	TSubclassOf<class AERNIntroBird> IntroBirdClass;

	// 페이드 인 지속 시간 (초)
	UPROPERTY(EditDefaultsOnly, Category = "Intro")
	float IntroFadeInDuration = 1.5f;

	// HideLoadingScreen 직후 자동으로 인트로 시작
	UPROPERTY(EditDefaultsOnly, Category = "Intro")
	bool bAutoStartIntroOnLoadingFinished = false;

	// 인트로 타이틀 위젯 클래스 (WBP_IntroTitle) — 타이밍은 위젯 내부 Animation에서 관리
	UPROPERTY(EditDefaultsOnly, Category = "Intro")
	TSubclassOf<UUserWidget> IntroTitleWidgetClass;

public:
	// 인트로용 게터들 (서브시스템에서 사용)
	UFUNCTION(BlueprintPure, Category = "Intro")
	TSubclassOf<AERNIntroBird> GetIntroBirdClass() const { return IntroBirdClass; }

	UFUNCTION(BlueprintPure, Category = "Intro")
	float GetIntroFadeInDuration() const { return IntroFadeInDuration; }

	UFUNCTION(BlueprintPure, Category = "Intro")
	bool ShouldAutoStartIntro() const { return bAutoStartIntroOnLoadingFinished; }

	UFUNCTION(BlueprintPure, Category = "Intro")
	TSubclassOf<UUserWidget> GetIntroTitleWidgetClass() const { return IntroTitleWidgetClass; }

private:
	// 상점 시스템 초기화
	void InitializeShopSystem();
};
