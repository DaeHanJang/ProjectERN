// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Character/Player/ERNPlayerState.h"
#include "Shop/Provider/ERNShopDataProvider.h"
#include "Enhancement/Provider/ERNUpgradeDataProvider.h"
#include "ERNGameInstance.generated.h"

class UERNDummyShopProvider;
class UUserWidget;

UCLASS()
class PROJECTERN_API UERNGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	UERNGameInstance();

	virtual void Init() override;

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

	// Delegate Handles
	FDelegateHandle OnCreateSessionCompleteDelegateHandle;
	FDelegateHandle OnFindSessionsCompleteDelegateHandle;
	FDelegateHandle OnJoinSessionCompleteDelegateHandle;
	FDelegateHandle OnDestroySessionCompleteDelegateHandle;

	// 생성할 세션 정보 임시 저장
	FString PendingServerName;
	int32 PendingMaxPlayers;

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
