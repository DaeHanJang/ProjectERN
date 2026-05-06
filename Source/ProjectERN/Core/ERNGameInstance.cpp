// Copyright Epic Games, Inc. All Rights Reserved.

#include "Core/ERNGameInstance.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "Online/OnlineSessionNames.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Shop/Provider/ERNDummyShopProvider.h"
#include "Shop/Provider/ERNNetworkShopProvider.h"
#include "Shop/Provider/ERNDataTableShopProvider.h"

UERNGameInstance::UERNGameInstance()
{
	PendingMaxPlayers = 3;
	CurrentCharacterType = ECharacterType::None;
}

void UERNGameInstance::Init()
{
	Super::Init();

	// 온라인 서브시스템 가져오기
	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	if (OnlineSubsystem)
	{
		SessionInterface = OnlineSubsystem->GetSessionInterface();

		if (SessionInterface.IsValid())
		{
			// Delegate 바인딩
			SessionInterface->OnCreateSessionCompleteDelegates.AddUObject(this, &UERNGameInstance::OnCreateSessionComplete);
			SessionInterface->OnFindSessionsCompleteDelegates.AddUObject(this, &UERNGameInstance::OnFindSessionsComplete);
			SessionInterface->OnJoinSessionCompleteDelegates.AddUObject(this, &UERNGameInstance::OnJoinSessionComplete);
			SessionInterface->OnDestroySessionCompleteDelegates.AddUObject(this, &UERNGameInstance::OnDestroySessionComplete);
		}
	}

	// 상점 시스템 초기화
	InitializeShopSystem();
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

	// 세션 설정
	FOnlineSessionSettings SessionSettings;
	SessionSettings.bIsLANMatch = true;
	SessionSettings.NumPublicConnections = MaxPlayers;
	SessionSettings.bShouldAdvertise = true;
	SessionSettings.bUsesPresence = false;
	SessionSettings.bAllowJoinInProgress = true;
	SessionSettings.bAllowJoinViaPresence = true;

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

		// 로비 레벨로 이동		
		if (GetWorld())
		{
			GetWorld()->ServerTravel("/Game/Assets/Maps/Map_Lobby?listen");
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

	// 세션 검색 설정
	SessionSearch = MakeShareable(new FOnlineSessionSearch());
	SessionSearch->bIsLanQuery = true;
	SessionSearch->MaxSearchResults = 20;

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

void UERNGameInstance::OnDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (bWasSuccessful)
	{
		UE_LOG(LogTemp, Log, TEXT("Session destroyed successfully!"));

		// 세션 파괴 후 Host 재시도 (있으면)
		if (!PendingServerName.IsEmpty())
		{
			HostSession(PendingServerName, PendingMaxPlayers);
			PendingServerName.Empty();
		}
	}
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
