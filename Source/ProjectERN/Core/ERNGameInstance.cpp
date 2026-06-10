// Copyright Epic Games, Inc. All Rights Reserved.

#include "Core/ERNGameInstance.h"
#include "Engine/Engine.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "Online/OnlineSessionNames.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Kismet/GameplayStatics.h"
#include "Subsystem/ERNCutsceneSubsystem.h"
#include "Shop/Provider/ERNDummyShopProvider.h"
#include "Shop/Provider/ERNNetworkShopProvider.h"
#include "Shop/Provider/ERNDataTableShopProvider.h"
#include "Enhancement/Provider/ERNUpgradeProvider.h"
#include "Core/ERNSaveSettings.h"
#include "GameFramework/GameUserSettings.h"
#include "Sound/SoundMix.h"
#include "DLSSLibrary.h"
#include "HAL/IConsoleManager.h"
#include "Sound/SoundClass.h"

static const FString ERNSettingsSlotName = TEXT("ERNSettings");

UERNGameInstance::UERNGameInstance()
{
	PendingMaxPlayers = 3;
	CurrentCharacterType = ECharacterType::None;
}

void UERNGameInstance::Init()
{
	Super::Init();

	// 현재 bUseSteam 값에 맞춰 GameNetDriver 정의 보장
	ConfigureNetDriverForMode();

	// bUseSteam에 맞는 OSS의 SessionInterface 취득 + 델리게이트 바인딩
	RebindSessionInterface();

	// 상점 시스템 초기화
	InitializeShopSystem();

	// 강화 시스템 초기화
	InitializeUpgradeSystem();

	// 설정 시스템 (해상도/DLSS 등) 준비
	// 저장된 설정 로드 + 적용
	LoadAndApplySettings();

	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UERNGameInstance::OnPostLoadMap);
}

void UERNGameInstance::OnPostLoadMap(UWorld* LoadedWorld)
{
	// 맵(레벨) 이동 시 상점 캐시 초기화 -> 다음 상점 오픈 시 새로운 품목 생성 유도
	if (ShopDataProvider)
	{
		IERNShopDataProvider::Execute_ClearCache(ShopDataProvider);
		UE_LOG(LogTemp, Log, TEXT("[ERNGameInstance] OnPostLoadMap 완료 - 상점 캐시 초기화"));
	}
}

void UERNGameInstance::LoadComplete(const float LoadTime, const FString& MapName)
{
	Super::LoadComplete(LoadTime, MapName);

	// 맵(레벨) 이동 시 상점 캐시 초기화 -> 다음 상점 오픈 시 새로운 품목 생성 유도
	if (ShopDataProvider)
	{
		IERNShopDataProvider::Execute_ClearCache(ShopDataProvider);
		UE_LOG(LogTemp, Log, TEXT("[ERNGameInstance] 맵 로드 완료(%s) - 상점 캐시 초기화"), *MapName);
	}
}

void UERNGameInstance::HostSession(FString ServerName, int32 MaxPlayers)
{
	if (!SessionInterface.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("SessionInterface is not valid!"));
		return;
	}

	// 기존 세션이 있으면 먼저 파괴
	auto ExistingSession = SessionInterface->GetNamedSession(NAME_GameSession);
	if (ExistingSession)
	{
		PendingServerName = ServerName;
		PendingMaxPlayers = MaxPlayers;
		SessionInterface->DestroySession(NAME_GameSession);
		return;
	}

	// 세션 설정 (bUseSteam에 따라 LAN/Steam 모드 분기)
	FOnlineSessionSettings SessionSettings;
	SessionSettings.bIsLANMatch = !bUseSteam;
	SessionSettings.NumPublicConnections = MaxPlayers;
	SessionSettings.bShouldAdvertise = true;
	SessionSettings.bUsesPresence = bUseSteam;            // Steam 모드에서만 Friend Presence
	SessionSettings.bAllowInvites = bUseSteam;            // Steam Overlay 초대 허용
	SessionSettings.bAllowJoinInProgress = true;
	SessionSettings.bAllowJoinViaPresence = true;
	SessionSettings.bUseLobbiesIfAvailable = bUseSteam;   // Steam Lobby 사용 (친구 초대 안정성)

	// 커스텀 세션 정보 (서버 이름)
	SessionSettings.Set(FName("SERVER_NAME"), ServerName, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

	// 세션 생성
	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	SessionInterface->CreateSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, SessionSettings);
}

void UERNGameInstance::OnCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (bWasSuccessful)
	{
		UE_LOG(LogTemp, Log, TEXT("Session created successfully!"));

		// 스탠드얼론 → 리슨 서버 전환은 OpenLevel + "listen" 옵션 필수
		// ServerTravel은 기존 NetDriver가 있을 때만 ?listen이 유효함
		if (GetWorld())
		{
			// 로비 진입 로딩화면 (호스트) — OpenLevel 로드 동안 화면 가림
			if (UERNCutsceneSubsystem* CutsceneSubsystem = GetSubsystem<UERNCutsceneSubsystem>())
			{
				CutsceneSubsystem->ShowLoadingScreen();
			}

			UGameplayStatics::OpenLevel(GetWorld(),
				FName(TEXT("/Game/Assets/Maps/Map_Lobby")),
				true,
				TEXT("listen"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to create session!"));
	}
}

void UERNGameInstance::FindSessions(FString SearchQuery)
{
	if (!SessionInterface.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("SessionInterface is not valid!"));
		return;
	}

	// 검색어 저장
	CurrentSearchQuery = SearchQuery;

	// 세션 검색 설정 (bUseSteam에 따라 LAN/Steam 모드 분기)
	SessionSearch = MakeShareable(new FOnlineSessionSearch());
	SessionSearch->bIsLanQuery = !bUseSteam;
	SessionSearch->MaxSearchResults = 20;

	// Steam 모드: Lobby 검색 활성화 (HostSession의 bUseLobbiesIfAvailable=true와 짝)
	if (bUseSteam)
	{
		SessionSearch->QuerySettings.Set(SEARCH_LOBBIES, true, EOnlineComparisonOp::Equals);
	}

	UE_LOG(LogTemp, Log, TEXT("Starting session search with query: '%s'..."), *SearchQuery);

	// 세션 검색 시작
	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	SessionInterface->FindSessions(*LocalPlayer->GetPreferredUniqueNetId(), SessionSearch.ToSharedRef());
}

void UERNGameInstance::OnFindSessionsComplete(bool bWasSuccessful)
{
	SessionSearchResults.Empty();
	SessionNames.Empty();
	FilteredSessionIndices.Empty();

	UE_LOG(LogTemp, Log, TEXT("OnFindSessionsComplete - Success: %s"), bWasSuccessful ? TEXT("true") : TEXT("false"));

	if (bWasSuccessful && SessionSearch.IsValid())
	{
		UE_LOG(LogTemp, Log, TEXT("Found %d sessions"), SessionSearch->SearchResults.Num());

		int32 FilteredCount = 0;

		// 검색 결과를 배열에 저장 (필터링 적용)
		for (int32 i = 0; i < SessionSearch->SearchResults.Num(); i++)
		{
			FString ServerName;
			SessionSearch->SearchResults[i].Session.SessionSettings.Get(FName("SERVER_NAME"), ServerName);

			// 검색어 필터링: 검색어가 비어있으면 모든 세션, 있으면 이름에 포함된 세션만
			bool bMatchesFilter = CurrentSearchQuery.IsEmpty() || ServerName.Contains(CurrentSearchQuery);

			if (!bMatchesFilter)
			{
				UE_LOG(LogTemp, Log, TEXT("Session %d filtered out: '%s' does not match '%s'"), i, *ServerName, *CurrentSearchQuery);
				continue;
			}

			int32 CurrentPlayers = SessionSearch->SearchResults[i].Session.SessionSettings.NumPublicConnections - SessionSearch->SearchResults[i].Session.NumOpenPublicConnections;
			int32 MaxPlayers = SessionSearch->SearchResults[i].Session.SessionSettings.NumPublicConnections;

			FString SessionInfo = FString::Printf(TEXT("%s (%d/%d)"), *ServerName, CurrentPlayers, MaxPlayers);
			SessionSearchResults.Add(SessionInfo);
			SessionNames.Add(ServerName);
			FilteredSessionIndices.Add(i); // 원본 인덱스 저장

			UE_LOG(LogTemp, Log, TEXT("Filtered session %d: %s (original index: %d) - Ping: %d"), FilteredCount, *SessionInfo, i, SessionSearch->SearchResults[i].PingInMs);
			FilteredCount++;
		}

		UE_LOG(LogTemp, Log, TEXT("Total sessions found: %d, Matching filter: %d"), SessionSearch->SearchResults.Num(), FilteredCount);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to find sessions! SearchValid: %s"), SessionSearch.IsValid() ? TEXT("true") : TEXT("false"));
	}
}

void UERNGameInstance::JoinSessionByIndex(int32 SessionIndex)
{
	if (!SessionInterface.IsValid() || !SessionSearch.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("Cannot join session!"));
		return;
	}

	// 필터링된 인덱스 범위 확인
	if (SessionIndex < 0 || SessionIndex >= FilteredSessionIndices.Num())
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid filtered session index: %d (max: %d)"), SessionIndex, FilteredSessionIndices.Num() - 1);
		return;
	}

	// 필터링된 인덱스를 원본 인덱스로 변환
	int32 OriginalIndex = FilteredSessionIndices[SessionIndex];

	// 원본 인덱스 유효성 확인
	if (OriginalIndex < 0 || OriginalIndex >= SessionSearch->SearchResults.Num())
	{
		UE_LOG(LogTemp, Warning, TEXT("Invalid original session index: %d"), OriginalIndex);
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("Joining session: filtered index %d -> original index %d"), SessionIndex, OriginalIndex);

	// 기존 세션 잔존 시 파괴 후 OnDestroySessionComplete에서 이어서 처리
	if (SessionInterface->GetNamedSession(NAME_GameSession))
	{
		UE_LOG(LogTemp, Log, TEXT("기존 세션 발견 — 파괴 후 재시도"));
		PendingJoinIndex = SessionIndex;
		bHasPendingJoin = true;
		SessionInterface->DestroySession(NAME_GameSession);
		return;
	}

	// 세션 참가
	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	SessionInterface->JoinSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, SessionSearch->SearchResults[OriginalIndex]);
}

void UERNGameInstance::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	if (Result == EOnJoinSessionCompleteResult::Success)
	{
		UE_LOG(LogTemp, Log, TEXT("Joined session successfully!"));

		// 서버 연결
		FString ConnectInfo;
		if (SessionInterface->GetResolvedConnectString(SessionName, ConnectInfo))
		{
			APlayerController* PC = GetWorld()->GetFirstPlayerController();
			if (PC)
			{
				// 로비 진입 로딩화면 (클라) — ClientTravel 연결/로드 동안 화면 가림
				if (UERNCutsceneSubsystem* CutsceneSubsystem = GetSubsystem<UERNCutsceneSubsystem>())
				{
					CutsceneSubsystem->ShowLoadingScreen();
				}

				PC->ClientTravel(ConnectInfo, TRAVEL_Absolute);
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Failed to join session!"));
	}
}

void UERNGameInstance::LeaveSession()
{
	if (SessionInterface.IsValid())
	{
		SessionInterface->DestroySession(NAME_GameSession);
	}
}

void UERNGameInstance::ConfigureNetDriverForMode()
{
	if (!GEngine) return;

	const FName GameNetDriverDef(TEXT("GameNetDriver"));

	GEngine->NetDriverDefinitions.RemoveAll([&](const FNetDriverDefinition& Def)
	{
		return Def.DefName == GameNetDriverDef;
	});

	FNetDriverDefinition NewDef;
	NewDef.DefName = GameNetDriverDef;
	if (bUseSteam)
	{
		NewDef.DriverClassName = FName(TEXT("/Script/SteamSockets.SteamSocketsNetDriver"));
		NewDef.DriverClassNameFallback = FName(TEXT("OnlineSubsystemUtils.IpNetDriver"));
	}
	else
	{
		NewDef.DriverClassName = FName(TEXT("OnlineSubsystemUtils.IpNetDriver"));
		NewDef.DriverClassNameFallback = FName(TEXT("OnlineSubsystemUtils.IpNetDriver"));
	}
	GEngine->NetDriverDefinitions.Add(NewDef);

	UE_LOG(LogTemp, Log, TEXT("[NetDriver] %s 모드 → %s"),
		bUseSteam ? TEXT("Steam") : TEXT("LAN"),
		*NewDef.DriverClassName.ToString());
}

void UERNGameInstance::SetUseSteam(bool bNewUseSteam)
{
	if (bUseSteam == bNewUseSteam)
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[Network] 모드 전환: %s → %s — 기존 세션 정리"),
		bUseSteam ? TEXT("Steam") : TEXT("LAN"),
		bNewUseSteam ? TEXT("Steam") : TEXT("LAN"));

	// 기존 세션 파괴 (있으면)
	if (SessionInterface.IsValid() && SessionInterface->GetNamedSession(NAME_GameSession))
	{
		SessionInterface->DestroySession(NAME_GameSession);
	}

	// 검색 결과/Pending 상태 클리어
	SessionSearchResults.Empty();
	SessionNames.Empty();
	FilteredSessionIndices.Empty();
	CurrentSearchQuery.Empty();
	SessionSearch.Reset();
	PendingServerName.Empty();
	bHasPendingJoin = false;
	PendingJoinIndex = -1;

	// 플래그 전환
	bUseSteam = bNewUseSteam;

	// NetDriver 정의 재구성
	ConfigureNetDriverForMode();

	// SessionInterface도 새 OSS의 것으로 재취득 (메뉴 체크박스가 진짜로 OSS를 바꾸도록)
	RebindSessionInterface();
}

void UERNGameInstance::RebindSessionInterface()
{
	// 이전 SessionInterface에 바인딩된 델리게이트 제거 (핸들 기반)
	if (SessionInterface.IsValid())
	{
		if (OnCreateSessionCompleteDelegateHandle.IsValid())
			SessionInterface->OnCreateSessionCompleteDelegates.Remove(OnCreateSessionCompleteDelegateHandle);
		if (OnFindSessionsCompleteDelegateHandle.IsValid())
			SessionInterface->OnFindSessionsCompleteDelegates.Remove(OnFindSessionsCompleteDelegateHandle);
		if (OnJoinSessionCompleteDelegateHandle.IsValid())
			SessionInterface->OnJoinSessionCompleteDelegates.Remove(OnJoinSessionCompleteDelegateHandle);
		if (OnDestroySessionCompleteDelegateHandle.IsValid())
			SessionInterface->OnDestroySessionCompleteDelegates.Remove(OnDestroySessionCompleteDelegateHandle);
		if (OnSessionUserInviteAcceptedDelegateHandle.IsValid())
			SessionInterface->OnSessionUserInviteAcceptedDelegates.Remove(OnSessionUserInviteAcceptedDelegateHandle);

		OnCreateSessionCompleteDelegateHandle.Reset();
		OnFindSessionsCompleteDelegateHandle.Reset();
		OnJoinSessionCompleteDelegateHandle.Reset();
		OnDestroySessionCompleteDelegateHandle.Reset();
		OnSessionUserInviteAcceptedDelegateHandle.Reset();
	}

	// bUseSteam에 따라 OSS를 명시적으로 선택 (DefaultPlatformService 무시)
	const FName OSSName = bUseSteam ? STEAM_SUBSYSTEM : NULL_SUBSYSTEM;
	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get(OSSName);
	if (!OnlineSubsystem)
	{
		UE_LOG(LogTemp, Warning, TEXT("[OSS] %s 모듈 사용 불가 — SessionInterface 비활성화"), *OSSName.ToString());
		SessionInterface = nullptr;
		return;
	}

	SessionInterface = OnlineSubsystem->GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[OSS] %s SessionInterface 취득 실패"), *OSSName.ToString());
		return;
	}

	// 새 SessionInterface에 델리게이트 등록 + 핸들 저장
	OnCreateSessionCompleteDelegateHandle =
		SessionInterface->OnCreateSessionCompleteDelegates.AddUObject(this, &UERNGameInstance::OnCreateSessionComplete);
	OnFindSessionsCompleteDelegateHandle =
		SessionInterface->OnFindSessionsCompleteDelegates.AddUObject(this, &UERNGameInstance::OnFindSessionsComplete);
	OnJoinSessionCompleteDelegateHandle =
		SessionInterface->OnJoinSessionCompleteDelegates.AddUObject(this, &UERNGameInstance::OnJoinSessionComplete);
	OnDestroySessionCompleteDelegateHandle =
		SessionInterface->OnDestroySessionCompleteDelegates.AddUObject(this, &UERNGameInstance::OnDestroySessionComplete);
	OnSessionUserInviteAcceptedDelegateHandle =
		SessionInterface->OnSessionUserInviteAcceptedDelegates.AddUObject(this, &UERNGameInstance::OnSessionUserInviteAccepted);

	UE_LOG(LogTemp, Log, TEXT("[OSS] %s SessionInterface 활성화"), *OSSName.ToString());
}

void UERNGameInstance::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (bWasSuccessful)
	{
		UE_LOG(LogTemp, Log, TEXT("Session destroyed successfully!"));

		// 세션 파괴 후 Join 재시도 (Host 재시도보다 우선)
		if (bHasPendingJoin)
		{
			const int32 IndexToRetry = PendingJoinIndex;
			bHasPendingJoin = false;
			PendingJoinIndex = -1;
			JoinSessionByIndex(IndexToRetry);
			return;
		}

		// 세션 파괴 후 Host 재시도 (있으면)
		if (!PendingServerName.IsEmpty())
		{
			HostSession(PendingServerName, PendingMaxPlayers);
			PendingServerName.Empty();
		}
	}
}

// Steam Overlay에서 친구가 "Join Game" 클릭 시 자동 호출
void UERNGameInstance::OnSessionUserInviteAccepted(
	const bool bWasSuccessful,
	const int32 ControllerId,
	FUniqueNetIdPtr UserId,
	const FOnlineSessionSearchResult& InviteResult)
{
	if (!bWasSuccessful)
	{
		UE_LOG(LogTemp, Warning, TEXT("Steam invite accepted but bWasSuccessful=false"));
		return;
	}

	if (!UserId.IsValid() || !InviteResult.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("Steam invite accepted but UserId or InviteResult invalid"));
		return;
	}

	if (!SessionInterface.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("SessionInterface not valid in invite accept"));
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("Steam invite accepted - joining session..."));

	// 기존 세션 있으면 정리 후 조인 (LeaveSession은 비동기라 단순화: 바로 조인)
	if (SessionInterface->GetNamedSession(NAME_GameSession))
	{
		SessionInterface->DestroySession(NAME_GameSession);
	}

	SessionInterface->JoinSession(*UserId, NAME_GameSession, InviteResult);
}

void UERNGameInstance::JoinByIP(FString IPAddress)
{
	APlayerController* PC = GetWorld()->GetFirstPlayerController();
	if (PC)
	{
		FString TravelURL = IPAddress;
		if (!IPAddress.Contains(":"))
		{
			TravelURL += ":7777";
		}

		UE_LOG(LogTemp, Log, TEXT("Joining by IP: %s"), *TravelURL);
		PC->ClientTravel(TravelURL, TRAVEL_Absolute);
	}
}

// ===== 상점 시스템 =====

void UERNGameInstance::InitializeShopSystem()
{
	// 1. 블루프린트에 할당된 클래스를 기반으로 Provider 객체 생성
	if (DataTableProviderClass)
	{
		ShopDataProvider = NewObject<UObject>(this, DataTableProviderClass);
		UE_LOG(LogTemp, Log, TEXT("[GameInstance] BP_DataTableShopProvider 생성 완료"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[GameInstance] DataTableProviderClass가 할당되지 않았습니다! (에디터에서 BP 연결 필요)"));
		ShopDataProvider = NewObject<UERNDataTableShopProvider>(this); // 에러 방지용 기본 생성
	}

	// 2. 생성된 Provider 초기화 (페이즈 3의 캐싱 로직 실행)
	if (ShopDataProvider)
	{
		IERNShopDataProvider* Provider = Cast<IERNShopDataProvider>(ShopDataProvider);
		if (Provider)
		{
			IERNShopDataProvider::Execute_Initialize(ShopDataProvider, this);
			UE_LOG(LogTemp, Log, TEXT("[GameInstance] 상점 시스템 초기화(캐싱) 완료"));
		}
	}
}

TScriptInterface<IERNShopDataProvider> UERNGameInstance::GetShopDataProvider() const
{
	TScriptInterface<IERNShopDataProvider> Result;
	if (ShopDataProvider)
	{
		Result.SetObject(ShopDataProvider);
		Result.SetInterface(Cast<IERNShopDataProvider>(ShopDataProvider));
	}
	return Result;
}

// ===== 강화 시스템 =====

void UERNGameInstance::InitializeUpgradeSystem()
{
	if (UpgradeProviderClass)
	{
		UpgradeDataProvider = NewObject<UObject>(this, UpgradeProviderClass);
		UE_LOG(LogTemp, Log, TEXT("[GameInstance] BP_UpgradeProvider 생성 완료"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[GameInstance] UpgradeProviderClass가 할당되지 않았습니다! (에디터에서 BP 연결 필요)"));
		UpgradeDataProvider = NewObject<UERNUpgradeProvider>(this); // 에러 방지용 기본 생성
	}

	if (UpgradeDataProvider)
	{
		IERNUpgradeDataProvider* Provider = Cast<IERNUpgradeDataProvider>(UpgradeDataProvider);
		if (Provider)
		{
			IERNUpgradeDataProvider::Execute_Initialize(UpgradeDataProvider, this);
			UE_LOG(LogTemp, Log, TEXT("[GameInstance] 강화 시스템 초기화(캐싱) 완료"));
		}
	}
}

TScriptInterface<IERNUpgradeDataProvider> UERNGameInstance::GetUpgradeDataProvider() const
{
	TScriptInterface<IERNUpgradeDataProvider> Result;
	if (UpgradeDataProvider)
	{
		Result.SetObject(UpgradeDataProvider);
		Result.SetInterface(Cast<IERNUpgradeDataProvider>(UpgradeDataProvider));
	}
	return Result;
}

// ===== 설정: 저장/로드 =====

void UERNGameInstance::SaveSettings()
{
	if (CachedSettings)
	{
		UGameplayStatics::SaveGameToSlot(CachedSettings, ERNSettingsSlotName, 0);
	}
}

void UERNGameInstance::LoadAndApplySettings()
{
	if (UGameplayStatics::DoesSaveGameExist(ERNSettingsSlotName, 0))
	{
		CachedSettings = Cast<UERNSaveSettings>(UGameplayStatics::LoadGameFromSlot(ERNSettingsSlotName, 0));
	}

	if (!CachedSettings)
	{
		CachedSettings = Cast<UERNSaveSettings>(UGameplayStatics::CreateSaveGameObject(UERNSaveSettings::StaticClass()));
	}

	// 오디오 볼륨 적용
	if (MasterSoundMix)
	{
		UGameplayStatics::PushSoundMixModifier(GetWorld(), MasterSoundMix);
		ApplyAudioVolume(MasterSoundClass, CachedSettings->MasterVolume);
		ApplyAudioVolume(MusicSoundClass, CachedSettings->MusicVolume);
		ApplyAudioVolume(SFXSoundClass, CachedSettings->SFXVolume);
	}

	// 밝기 적용
	if (GEngine)
	{
		float Gamma = FMath::GetMappedRangeValueClamped(
			FVector2D(0.0f, 100.0f), FVector2D(1.5f, 3.0f), CachedSettings->Brightness);
		GEngine->DisplayGamma = Gamma;
	}

	// DLSS 복원 (저장된 켜짐/모드 적용)
	ApplyDLSS();
}

// ===== 설정: 오디오 =====

void UERNGameInstance::ApplyAudioVolume(USoundClass* SoundClass, float Volume)
{
	if (!MasterSoundMix || !SoundClass) return;

	UWorld* World = GetWorld();
	if (!World) return;

	UGameplayStatics::SetSoundMixClassOverride(World, MasterSoundMix, SoundClass,
		FMath::Clamp(Volume, 0.0f, 1.0f), 1.0f, 0.0f, true);
}

void UERNGameInstance::SetMasterVolume(float Volume)
{
	if (!CachedSettings) return;
	CachedSettings->MasterVolume = FMath::Clamp(Volume, 0.0f, 1.0f);
	ApplyAudioVolume(MasterSoundClass, CachedSettings->MasterVolume);
	SaveSettings();
}

float UERNGameInstance::GetMasterVolume() const
{
	return CachedSettings ? CachedSettings->MasterVolume : 1.0f;
}

void UERNGameInstance::SetMusicVolume(float Volume)
{
	if (!CachedSettings) return;
	CachedSettings->MusicVolume = FMath::Clamp(Volume, 0.0f, 1.0f);
	ApplyAudioVolume(MusicSoundClass, CachedSettings->MusicVolume);
	SaveSettings();
}

float UERNGameInstance::GetMusicVolume() const
{
	return CachedSettings ? CachedSettings->MusicVolume : 1.0f;
}

void UERNGameInstance::SetSFXVolume(float Volume)
{
	if (!CachedSettings) return;
	CachedSettings->SFXVolume = FMath::Clamp(Volume, 0.0f, 1.0f);
	ApplyAudioVolume(SFXSoundClass, CachedSettings->SFXVolume);
	SaveSettings();
}

float UERNGameInstance::GetSFXVolume() const
{
	return CachedSettings ? CachedSettings->SFXVolume : 1.0f;
}

// ===== 설정: 비디오 =====

void UERNGameInstance::SetWindowMode(int32 Mode)
{
	UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
	if (!Settings) return;

	EWindowMode::Type WinMode;
	switch (Mode)
	{
	case 0: WinMode = EWindowMode::Fullscreen; break;
	case 1: WinMode = EWindowMode::WindowedFullscreen; break;
	case 2: WinMode = EWindowMode::Windowed; break;
	default: WinMode = EWindowMode::WindowedFullscreen; break;
	}

	Settings->SetFullscreenMode(WinMode);

	// Fullscreen 모드는 데스크탑 해상도와 일치해야 정상 작동
	if (WinMode == EWindowMode::Fullscreen)
	{
		Settings->SetScreenResolution(Settings->GetDesktopResolution());
	}
}

int32 UERNGameInstance::GetWindowMode() const
{
	UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
	if (!Settings) return 1;

	switch (Settings->GetFullscreenMode())
	{
	case EWindowMode::Fullscreen: return 0;
	case EWindowMode::WindowedFullscreen: return 1;
	case EWindowMode::Windowed: return 2;
	default: return 1;
	}
}

TArray<FIntPoint> UERNGameInstance::GetSupportedResolutions() const
{
	UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
	FIntPoint DesktopRes = Settings ? Settings->GetDesktopResolution() : FIntPoint(1920, 1080);

	const TArray<FIntPoint> AllResolutions = {
		FIntPoint(3840, 2160),
		FIntPoint(2560, 1440),
		FIntPoint(1920, 1080),
		FIntPoint(1600, 900),
		FIntPoint(1280, 720)
	};

	TArray<FIntPoint> Resolutions;
	for (const FIntPoint& Res : AllResolutions)
	{
		if (Res.X <= DesktopRes.X && Res.Y <= DesktopRes.Y)
		{
			Resolutions.Add(Res);
		}
	}
	return Resolutions;
}

void UERNGameInstance::SetResolution(FIntPoint Resolution)
{
	UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
	if (Settings) Settings->SetScreenResolution(Resolution);
}

FIntPoint UERNGameInstance::GetCurrentResolution() const
{
	UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
	return Settings ? Settings->GetScreenResolution() : FIntPoint(1920, 1080);
}

FString UERNGameInstance::FormatResolution(FIntPoint Resolution)
{
	return FString::Printf(TEXT("%d x %d"), Resolution.X, Resolution.Y);
}

void UERNGameInstance::SetBrightness(float Value)
{
	if (!CachedSettings) return;
	CachedSettings->Brightness = FMath::Clamp(Value, 0.0f, 100.0f);

	if (GEngine)
	{
		float Gamma = FMath::GetMappedRangeValueClamped(
			FVector2D(0.0f, 100.0f), FVector2D(1.5f, 3.0f), CachedSettings->Brightness);
		GEngine->DisplayGamma = Gamma;
	}

	SaveSettings();
}

float UERNGameInstance::GetBrightness() const
{
	return CachedSettings ? CachedSettings->Brightness : 50.0f;
}

void UERNGameInstance::SetOverallQuality(int32 Level)
{
	UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
	if (Settings) Settings->SetOverallScalabilityLevel(Level);
}
int32 UERNGameInstance::GetOverallQuality() const
{
	UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
	return Settings ? Settings->GetOverallScalabilityLevel() : 3;
}

void UERNGameInstance::SetShadowQuality(int32 Level)
{
	UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
	if (Settings) Settings->SetShadowQuality(Level);
}
int32 UERNGameInstance::GetShadowQuality() const
{
	UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
	return Settings ? Settings->GetShadowQuality() : 3;
}

void UERNGameInstance::SetAntiAliasingQuality(int32 Level)
{
	UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
	if (Settings) Settings->SetAntiAliasingQuality(Level);
}
int32 UERNGameInstance::GetAntiAliasingQuality() const
{
	UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
	return Settings ? Settings->GetAntiAliasingQuality() : 3;
}

void UERNGameInstance::SetTextureQuality(int32 Level)
{
	UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
	if (Settings) Settings->SetTextureQuality(Level);
}
int32 UERNGameInstance::GetTextureQuality() const
{
	UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
	return Settings ? Settings->GetTextureQuality() : 3;
}

void UERNGameInstance::SetViewDistanceQuality(int32 Level)
{
	UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
	if (Settings) Settings->SetViewDistanceQuality(Level);
}
int32 UERNGameInstance::GetViewDistanceQuality() const
{
	UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
	return Settings ? Settings->GetViewDistanceQuality() : 3;
}

void UERNGameInstance::SetEffectsQuality(int32 Level)
{
	UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
	if (Settings) Settings->SetVisualEffectQuality(Level);
}
int32 UERNGameInstance::GetEffectsQuality() const
{
	UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
	return Settings ? Settings->GetVisualEffectQuality() : 3;
}

void UERNGameInstance::ApplyVideoSettings()
{
	UGameUserSettings* Settings = UGameUserSettings::GetGameUserSettings();
	if (!Settings) return;

	Settings->ApplySettings(false);
	Settings->SaveSettings();

	// 해상도/스케일러빌리티가 스크린퍼센트를 건드리므로 DLSS를 마지막에 재적용 (DLSS가 최종 결정)
	ApplyDLSS();
}

// ===== 설정: DLSS =====

bool UERNGameInstance::IsDLSSAvailable() const
{
	return UDLSSLibrary::IsDLSSSupported();
}

void UERNGameInstance::SetDLSSEnabled(bool bEnabled)
{
	if (CachedSettings) CachedSettings->bDLSSEnabled = bEnabled;
	ApplyDLSS();
	SaveSettings();
}

bool UERNGameInstance::IsDLSSEnabled() const
{
	return CachedSettings ? CachedSettings->bDLSSEnabled : false;
}

TArray<UDLSSMode> UERNGameInstance::GetSupportedDLSSModes() const
{
	TArray<UDLSSMode> Modes = UDLSSLibrary::GetSupportedDLSSModes();
	// 체크박스가 on/off 담당하므로 드롭다운에선 Off/Auto 제외
	Modes.Remove(UDLSSMode::Off);
	Modes.Remove(UDLSSMode::Auto);
	return Modes;
}

void UERNGameInstance::SetDLSSMode(UDLSSMode Mode)
{
	if (CachedSettings) CachedSettings->DLSSMode = static_cast<int32>(Mode);
	ApplyDLSS();
	SaveSettings();
}

UDLSSMode UERNGameInstance::GetDLSSMode() const
{
	return CachedSettings ? static_cast<UDLSSMode>(CachedSettings->DLSSMode) : UDLSSMode::Quality;
}

void UERNGameInstance::ApplyDLSS()
{
	if (!UDLSSLibrary::IsDLSSSupported())
	{
		return;
	}

	static IConsoleVariable* CVarScreenPercentage = IConsoleManager::Get().FindConsoleVariable(TEXT("r.ScreenPercentage"));

	// 꺼짐 → DLSS off + 스크린퍼센트 100 복원
	const bool bEnabled = CachedSettings && CachedSettings->bDLSSEnabled;
	if (!bEnabled)
	{
		UDLSSLibrary::EnableDLSS(false);
		if (CVarScreenPercentage) CVarScreenPercentage->Set(100.f, ECVF_SetByGameSetting);
		return;
	}

	// 켜짐 → DLSS on + 모드별 최적 스크린퍼센트 적용
	UDLSSLibrary::EnableDLSS(true);

	const UDLSSMode Mode = CachedSettings ? static_cast<UDLSSMode>(CachedSettings->DLSSMode) : UDLSSMode::Quality;

	const FIntPoint Res = GetCurrentResolution();
	bool bModeSupported = false;
	bool bFixedScreenPercentage = false;
	float OptimalScreenPercentage = 100.f;
	float MinScreenPercentage = 100.f;
	float MaxScreenPercentage = 100.f;
	float OptimalSharpness = 0.f;
	UDLSSLibrary::GetDLSSModeInformation(
		Mode, FVector2D(Res.X, Res.Y),
		bModeSupported, OptimalScreenPercentage, bFixedScreenPercentage,
		MinScreenPercentage, MaxScreenPercentage, OptimalSharpness);

	const float ScreenPercentage = (bModeSupported && OptimalScreenPercentage > 0.f) ? OptimalScreenPercentage : 100.f;
	if (CVarScreenPercentage) CVarScreenPercentage->Set(ScreenPercentage, ECVF_SetByGameSetting);
}
