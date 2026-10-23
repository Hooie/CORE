// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/CGameInstance.h"
#include "Framework/CGameState.h"
#include "Framework/CGameUserSettings.h"
#include "GameFramework/PlayerState.h"
#include "GenericPlatform/GenericPlatformHttp.h"
#include "HttpModule.h"
#include "Misc/PackageName.h"
#include "Network/CNetStatics.h"
#include "Player/LobbyPlayerController.h"
#include "Kismet/GameplayStatics.h"

void UCGameInstance::StartMatch()
{
	if (GetWorld()->GetNetMode() == ENetMode::NM_DedicatedServer || GetWorld()->GetNetMode() == ENetMode::NM_ListenServer)
	{
		if ( const AGameStateBase* LobbyGameState = GetWorld()->GetGameState() )
		{
			ExpectedMatchPlayerCount = LobbyGameState->PlayerArray.Num();
		}

		LoadLevelAndListen(GameLevel);
	}
}

int32 UCGameInstance::ConsumeExpectedMatchPlayerCount()
{
	const int32 Count = ExpectedMatchPlayerCount;
	ExpectedMatchPlayerCount = 0;
	
	return Count;
}

FString UCGameInstance::GetGameLevelPackageName() const
{
	return GameLevel.IsNull() ? FString() : GameLevel.GetLongPackageName();
}

FString UCGameInstance::GetGameLevelShortName() const
{
	const FString PackageName = GetGameLevelPackageName();

	return PackageName.IsEmpty() ? FString() : FPackageName::GetShortName(PackageName);
}

void UCGameInstance::Init()
{
	Super::Init();

	FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UCGameInstance::OnPostLoadMap);

	if ( GetWorld()->IsEditorWorld() )
		return;

	GEngine->OnNetworkFailure().AddUObject(this, &UCGameInstance::HandleNetworkFailure);

	if (UCNetStatics::IsSessionServer(this))
	{
		DesignatedHostUniqueIdStr = UCNetStatics::GetCommandLineArgAsString(UCNetStatics::GetHostUniqueIdKey());
		CreateSession();
	}
}

void UCGameInstance::ReturnToMainMenu()
{
	UE_LOG(LogTemp, Warning, TEXT("UCGameInstance::ReturnToMainMenu called - ignoring (custom flow handles this)"))
}

void UCGameInstance::Shutdown()
{
	FCoreUObjectDelegates::PostLoadMapWithWorld.RemoveAll(this);
	Super::Shutdown();
}

const FName UCGameInstance::GetMainMenuURL()
{
	const FName LevelURL = FName(*FPackageName::ObjectPathToPackageName(MainMenuLevel.ToString()));

	return LevelURL;
}

bool UCGameInstance::IsLoggedIn() const
{
	if (IOnlineIdentityPtr IdentityPtr = UCNetStatics::GetIdentityPtr())
	{
		return IdentityPtr->GetLoginStatus(0) == ELoginStatus::LoggedIn;
	}

	return false;
}

bool UCGameInstance::IsLoggingIn() const
{
	return LoggingInDelegateHandle.IsValid();
}

void UCGameInstance::ClientAccountPortalLogin()
{
	ClientLogin("AccountPortal", "", "");
}

void UCGameInstance::Logout()
{
	if ( IOnlineIdentityPtr IdentityPtr = UCNetStatics::GetIdentityPtr() )
	{
		if ( LoggingOutDelegateHandle.IsValid() )
		{
			IdentityPtr->OnLogoutCompleteDelegates->Remove(LoggingOutDelegateHandle);
			LoggingOutDelegateHandle.Reset();
		}

		LoggingOutDelegateHandle = IdentityPtr->OnLogoutCompleteDelegates->AddUObject(this, &UCGameInstance::LogoutCompleted);

		if ( !IdentityPtr->Logout(0) )
		{
			UE_LOG(LogTemp, Warning, TEXT("Logout Failed Right Away!"));

			if ( LoggingOutDelegateHandle.IsValid() )
			{
				IdentityPtr->OnLogoutCompleteDelegates->Remove(LoggingOutDelegateHandle);
				LoggingOutDelegateHandle.Reset();
			}

			OnLogoutCompleted.Broadcast(false);
		}
	}
	else
	{
		OnLogoutCompleted.Broadcast(false);
	}
}

void UCGameInstance::OnStart()
{
	Super::OnStart();

	if ( UCGameUserSettings* Settings = UCGameUserSettings::Get() )
		Settings->ApplyAudioVolumes(GetWorld());
}

void UCGameInstance::OnPostLoadMap(UWorld* LoadedWorld)
{
	if ( UCGameUserSettings* Settings = UCGameUserSettings::Get() )
		Settings->ApplyAudioVolumes(LoadedWorld);
}

void UCGameInstance::ClientLogin(const FString& Type, const FString& Id, const FString& Token)
{
	if (IOnlineIdentityPtr IdentityPtr = UCNetStatics::GetIdentityPtr())
	{
		if (LoggingInDelegateHandle.IsValid())
		{
			IdentityPtr->OnLoginCompleteDelegates->Remove(LoggingInDelegateHandle);
			LoggingInDelegateHandle.Reset();
		}

		LoggingInDelegateHandle = IdentityPtr->OnLoginCompleteDelegates->AddUObject(this, &UCGameInstance::LoginCompleted);

		if (!IdentityPtr->Login(0, FOnlineAccountCredentials(Type, Id, Token)))
		{
			UE_LOG(LogTemp, Warning, TEXT("Login Failed Right Away!"));

			if (LoggingInDelegateHandle.IsValid())
			{
				IdentityPtr->OnLoginCompleteDelegates->Remove(LoggingInDelegateHandle);
				LoggingInDelegateHandle.Reset();
			}

			OnLoginCompleted.Broadcast(false, "", "Login Failed Right Away!");
		}
	}
}

void UCGameInstance::LoginCompleted(int NumOfLocalPlayer, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error)
{
	if (IOnlineIdentityPtr IdentityPtr = UCNetStatics::GetIdentityPtr())
	{
		if (LoggingInDelegateHandle.IsValid())
		{
			IdentityPtr->OnLoginCompleteDelegates->Remove(LoggingInDelegateHandle);
			LoggingInDelegateHandle.Reset();
		}

		if (bWasSuccessful)
		{
			CachedPlayerNickname = IdentityPtr->GetPlayerNickname(UserId);
			UE_LOG(LogTemp, Warning, TEXT("Logged in Successfully as : %s"), *(CachedPlayerNickname));
		}

		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Logging in Failed as : %s"), *(Error));
		}

		OnLoginCompleted.Broadcast(bWasSuccessful, CachedPlayerNickname, Error);
	}

	else
	{
		OnLoginCompleted.Broadcast(false, "", "Can't find the Identity Pointer");
	}
}

void UCGameInstance::LogoutCompleted(int32 LocalUserNum, bool bWasSuccessful)
{
	if ( IOnlineIdentityPtr IdentityPtr = UCNetStatics::GetIdentityPtr() )
	{
		if ( LoggingOutDelegateHandle.IsValid() )
		{
			IdentityPtr->OnLogoutCompleteDelegates->Remove(LoggingOutDelegateHandle);
			LoggingOutDelegateHandle.Reset();
		}
	}

	if ( bWasSuccessful )
	{
		UE_LOG(LogTemp, Warning, TEXT("Logged out successfully"));
		CachedPlayerNickname.Reset();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Logout Failed"));
	}

	OnLogoutCompleted.Broadcast(bWasSuccessful);
}

void UCGameInstance::RequestCreateAndJoinSession(const FName& NewSessionName)
{
	UE_LOG(LogTemp, Warning, TEXT("Requesting Create and Join Session : %s "), *(NewSessionName.ToString()));

	FHttpRequestRef Request = FHttpModule::Get().CreateRequest();
	FGuid SessionSearchId = FGuid::NewGuid();

	PendingSessionRequest = Request;
	PendingSessionSearchId = SessionSearchId;

	FString CoordinatorURL = UCNetStatics::GetCoordinatorURL();

	FString URL = FString::Printf(TEXT("%s/Sessions"), *(CoordinatorURL));
	UE_LOG(LogTemp, Warning, TEXT("Sending Request Creation to URL : %s"), *(URL));

	Request->SetURL(URL);
	Request->SetVerb("POST");

	FString HostUniqueIdStr;
	if ( IOnlineIdentityPtr IdentityPtr = UCNetStatics::GetIdentityPtr() )
	{
		if ( FUniqueNetIdPtr LocalId = IdentityPtr->GetUniquePlayerId(0) )
		{
			HostUniqueIdStr = LocalId->ToString();
		}
	}

	Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));

	TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject);
	JsonObject->SetStringField(UCNetStatics::GetSessionNameKey().ToString(), NewSessionName.ToString());
	JsonObject->SetStringField(UCNetStatics::GetSessionSearchIdKey().ToString(), SessionSearchId.ToString());
	JsonObject->SetStringField(UCNetStatics::GetHostNicknameKey().ToString(), CachedPlayerNickname);
	JsonObject->SetStringField(UCNetStatics::GetHostUniqueIdKey().ToString(), HostUniqueIdStr);
	
	FString RequestBody;
	TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&RequestBody);
	FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

	Request->SetContentAsString(RequestBody);
	Request->OnProcessRequestComplete().BindUObject(this, &UCGameInstance::SessionCreationRequestCompleted, SessionSearchId);

	if (!Request->ProcessRequest())
	{
		UE_LOG(LogTemp, Warning, TEXT("Session Creation Request Failed Right Away!"));
	}
}

void UCGameInstance::CancelSessionCreation()
{
	UE_LOG(LogTemp, Warning, TEXT("Canceling Session Creation"));

	if ( PendingSessionRequest.IsValid() && PendingSessionRequest->GetStatus() == EHttpRequestStatus::Processing )
	{
		PendingSessionRequest->CancelRequest();

		UE_LOG(LogTemp, Warning, TEXT("HTTP Request Cancelled"));
	}
	PendingSessionRequest.Reset();

	SendCancelToCoordinator();

	StopAllSessionFindings();

	if (IOnlineSessionPtr SessionPtr = UCNetStatics::GetSessionPtr())
	{
		SessionPtr->OnFindSessionsCompleteDelegates.RemoveAll(this);
		SessionPtr->OnJoinSessionCompleteDelegates.RemoveAll(this);
	}

	StartGlobalSessionSearch();
}

void UCGameInstance::StartGlobalSessionSearch()
{
	UE_LOG(LogTemp, Warning, TEXT("Starting Global Session Search!"));

	GetWorld()->GetTimerManager().SetTimer(GlobalSessionSearchTimerHandle, this, &UCGameInstance::FindGlobalSessions, GlobalSessionSearchInterval, true, 0.f);
}

bool UCGameInstance::JoinSessionWithId(const FString& SessionIdStr)
{
	if (SessionSearch.IsValid())
	{
		const FOnlineSessionSearchResult* SessionSearchResult = SessionSearch->SearchResults.FindByPredicate(
			[=](const FOnlineSessionSearchResult& Result)
			{
				return Result.GetSessionIdStr() == SessionIdStr;
			}
		);

		if (SessionSearchResult)
		{
			JoinSessionWithSearchResult(*SessionSearchResult);
			return true;
		}
	}

	return false;
}

void UCGameInstance::RefreshSessionsNow()
{
	FindGlobalSessions();
}

FString UCGameInstance::ConsumePendingJoinRejectReason()
{
	FString Reason = PendingJoinRejectReason;
	PendingJoinRejectReason.Empty();

	return Reason;
}

void UCGameInstance::LeaveCurrentSessionAndReturnToMainMenu()
{
	IOnlineSessionPtr SessionPtr = UCNetStatics::GetSessionPtr();

	if ( SessionPtr && CurrentJoinedSessionName != NAME_None )
	{
		SessionPtr->OnDestroySessionCompleteDelegates.RemoveAll(this);
		SessionPtr->OnDestroySessionCompleteDelegates.AddUObject(this, &UCGameInstance::OnLeaveSessionCompleted);

		if ( !SessionPtr->DestroySession(CurrentJoinedSessionName) )
		{
			UE_LOG(LogTemp, Warning, TEXT("Leave Session Failed Right Away, Traveling to Main Menu Anyway"));
			SessionPtr->OnDestroySessionCompleteDelegates.RemoveAll(this);
			OnLeaveSessionCompleted(CurrentJoinedSessionName, false);
		}
	}
	else
	{
		OnLeaveSessionCompleted(NAME_None, true);
	}
}

void UCGameInstance::SessionCreationRequestCompleted(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully, FGuid SessionSearchId)
{
	if (!bConnectedSuccessfully)
	{
		UE_LOG(LogTemp, Warning, TEXT("Connection Responded with connection was not successful!"));
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("Connection to Coordinator successfully!"));

	int32 ResponseCode = Response->GetResponseCode();
	if (ResponseCode != 200)
	{
		UE_LOG(LogTemp, Warning, TEXT("Session Creation Failed with code : %d"), ResponseCode);
		return;
	}

	FString ResponseStr = Response->GetContentAsString();
	TSharedPtr<FJsonObject> JsonObject;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ResponseStr);
	
	int32 Port = 0;
	if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
	{
		Port = JsonObject->GetIntegerField(*(UCNetStatics::GetPortKey().ToString()));
	}

	UE_LOG(LogTemp, Warning, TEXT("Connected to Coordinator Successfully and the new session created is on port : %d"), Port);

	StartFindingCreatedSession(SessionSearchId);
}

void UCGameInstance::StartFindingCreatedSession(const FGuid& SessionSearchId)
{
	if (!SessionSearchId.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("Session Search ID is invalid, Can't start Finding Session!"));
		return;
	}

	StopAllSessionFindings();
	UE_LOG(LogTemp, Warning, TEXT("Start Finding Created Session with ID: %s"), *(SessionSearchId.ToString()));

	GetWorld()->GetTimerManager().SetTimer(FindCreatedSessionSearchTimerHandle,
		FTimerDelegate::CreateUObject(this, &UCGameInstance::FindCreatedSession, SessionSearchId),
		FindCreatedSessionSearchInterval,
		true,
		0.f
	);

	GetWorld()->GetTimerManager().SetTimer(FindCreatedSessionTimeoutTimerHandle, this, &UCGameInstance::FindCreatedSessionTimeout, FindCreatedSessionTimeoutDuration);
}

void UCGameInstance::StopAllSessionFindings()
{
	UE_LOG(LogTemp, Warning, TEXT("Stopping All Session Search"));

	StopFindingCreatedSession();
	StopGlobalSessionSearch();
}

void UCGameInstance::StopFindingCreatedSession()
{
	UE_LOG(LogTemp, Warning, TEXT("Stop finding created session"));
	GetWorld()->GetTimerManager().ClearTimer(FindCreatedSessionSearchTimerHandle);
	GetWorld()->GetTimerManager().ClearTimer(FindCreatedSessionTimeoutTimerHandle);

	if (IOnlineSessionPtr SessionPtr = UCNetStatics::GetSessionPtr())
	{
		SessionPtr->OnFindSessionsCompleteDelegates.RemoveAll(this);
		SessionPtr->OnJoinSessionCompleteDelegates.RemoveAll(this);
	}
}

void UCGameInstance::StopGlobalSessionSearch()
{
	UE_LOG(LogTemp, Warning, TEXT("Stop Global session search"));

	if (GlobalSessionSearchTimerHandle.IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(GlobalSessionSearchTimerHandle);
	}

	IOnlineSessionPtr SessionPtr = UCNetStatics::GetSessionPtr();
	if (SessionPtr)
	{
		SessionPtr->OnFindSessionsCompleteDelegates.RemoveAll(this);
	}
}

void UCGameInstance::FindGlobalSessions()
{
	UE_LOG(LogTemp, Warning, TEXT("--------Retrying Global Session Search --------"));

	IOnlineSessionPtr SessionPtr = UCNetStatics::GetSessionPtr();
	if (!SessionPtr)
	{
		UE_LOG(LogTemp, Warning, TEXT("Can't find Session Interface, Wait for the next Global Session Search"));
		return;
	}

	SessionSearch = MakeShareable(new FOnlineSessionSearch);
	SessionSearch->bIsLanQuery = false;
	SessionSearch->MaxSearchResults = 20;
	
	SessionPtr->OnFindSessionsCompleteDelegates.RemoveAll(this);
	SessionPtr->OnFindSessionsCompleteDelegates.AddUObject(this, &UCGameInstance::GlobalSessionSearchCompleted);

	if (!SessionPtr->FindSessions(0, SessionSearch.ToSharedRef()))
	{
		UE_LOG(LogTemp, Warning, TEXT("Find Global Session Failed!!!!......"));
		SessionPtr->OnFindSessionsCompleteDelegates.RemoveAll(this);
	}
}

void UCGameInstance::GlobalSessionSearchCompleted(bool bWasSuccessful)
{
	if (bWasSuccessful)
	{
		OnGlobalSessionSearchCompleted.Broadcast(SessionSearch->SearchResults);
		for (const FOnlineSessionSearchResult& OnlineSessionSearchResult : SessionSearch->SearchResults)
		{
			FString SessionName = "Name_None";
			OnlineSessionSearchResult.Session.SessionSettings.Get<FString>(UCNetStatics::GetSessionNameKey(), SessionName);

			UE_LOG(LogTemp, Warning, TEXT("Found Session : %s after global session search"), *SessionName);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("global session search Completed Unsuccessfully!"));
	}

	IOnlineSessionPtr SessionPtr = UCNetStatics::GetSessionPtr();
	if (!SessionPtr)
	{
		SessionPtr->OnFindSessionsCompleteDelegates.RemoveAll(this);
	}
}

void UCGameInstance::HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString)
{
	UE_LOG(LogTemp, Warning, TEXT("Network Failure - Type : %d, ErrorString : %s"), ( int32 ) FailureType, *ErrorString);

	if ( IOnlineSessionPtr SessionPtr = UCNetStatics::GetSessionPtr() )
	{
		if ( CurrentJoinedSessionName != NAME_None )
		{
			SessionPtr->DestroySession(CurrentJoinedSessionName);
			CurrentJoinedSessionName = NAME_None;
		}
	}

	FString Reason = ErrorString.Contains(TEXT("HeroSelectionAlreadyStarted")) ? TEXT("이미 영웅 선택이 시작된 방입니다.") : TEXT("접속에 실패했습니다.");

	PendingJoinRejectReason = Reason; 
	OnJoinRejected.Broadcast(Reason);
}

void UCGameInstance::FindCreatedSession(FGuid SessionSearchId)
{
	UE_LOG(LogTemp, Warning, TEXT("Trying to find created session"));
	
	IOnlineSessionPtr SessionPtr = UCNetStatics::GetSessionPtr();
	if (!SessionPtr)
	{
		UE_LOG(LogTemp, Warning, TEXT("Can't find session ptr, cacenling session Search"));
		return;
	}

	SessionSearch = MakeShareable(new FOnlineSessionSearch);
	if (!SessionSearch)
	{
		UE_LOG(LogTemp, Warning, TEXT("Unable to create session search! Canceling session search"));
		return;
	}

	SessionSearch->bIsLanQuery = false;
	SessionSearch->MaxSearchResults = 1;
	SessionSearch->QuerySettings.Set(UCNetStatics::GetSessionSearchIdKey(), SessionSearchId.ToString(), EOnlineComparisonOp::Equals);

	SessionPtr->OnFindSessionsCompleteDelegates.RemoveAll(this);
	SessionPtr->OnFindSessionsCompleteDelegates.AddUObject(this, &UCGameInstance::FindCreateSessionCompleted);
	if (!SessionPtr->FindSessions(0, SessionSearch.ToSharedRef()))
	{
		UE_LOG(LogTemp, Warning, TEXT("Find Session Failed right away!..."));
		SessionPtr->OnFindSessionsCompleteDelegates.RemoveAll(this);
	}
}

void UCGameInstance::FindCreatedSessionTimeout()
{
	UE_LOG(LogTemp, Warning, TEXT("Find Created Session Timeout Reached"));
	
	StopFindingCreatedSession();
}

void UCGameInstance::FindCreateSessionCompleted(bool bWasSuccessful)
{
	if (!bWasSuccessful || SessionSearch->SearchResults.Num() == 0)
	{
		return;
	}

	StopFindingCreatedSession();
	JoinSessionWithSearchResult(SessionSearch->SearchResults[0]);
}

void UCGameInstance::JoinSessionWithSearchResult(const FOnlineSessionSearchResult& SearchResult)
{
	UE_LOG(LogTemp, Warning, TEXT("Joining Session with Search Result!!!"));

	IOnlineSessionPtr SessionPtr = UCNetStatics::GetSessionPtr();
	if (!SessionPtr)
	{
		UE_LOG(LogTemp, Warning, TEXT("Can't Find Session Ptr, Cancel Joining"));
		return;
	}

	FString SessionName = "";
	SearchResult.Session.SessionSettings.Get<FString>(UCNetStatics::GetSessionNameKey(), SessionName);

	const FOnlineSessionSetting* PortSetting = SearchResult.Session.SessionSettings.Settings.Find(UCNetStatics::GetPortKey());
	int64 Port = 7777;
	PortSetting->Data.GetValue(Port);

	UE_LOG(LogTemp, Warning, TEXT("Trying to join Session: %s, at port : %d"), *(SessionName), Port);
	SessionPtr->OnJoinSessionCompleteDelegates.RemoveAll(this);
	SessionPtr->OnJoinSessionCompleteDelegates.AddUObject(this, &UCGameInstance::JoinSessionCompleted, (int)Port);
	
	if (!SessionPtr->JoinSession(0, FName(SessionName), SearchResult))
	{
		UE_LOG(LogTemp, Warning, TEXT("Joining Session Failed!.........."));
		SessionPtr->OnJoinSessionCompleteDelegates.RemoveAll(this);
		OnJoinSessionFailed.Broadcast();
	}
}

void UCGameInstance::JoinSessionCompleted(FName SessionName, EOnJoinSessionCompleteResult::Type JoinResult, int Port)
{
	IOnlineSessionPtr SessionPtr = UCNetStatics::GetSessionPtr();
	if (!SessionPtr)
	{
		UE_LOG(LogTemp, Warning, TEXT("Join Session Completed, but Can't find session pointer"));
		OnJoinSessionFailed.Broadcast();
		return;
	}

	if (JoinResult == EOnJoinSessionCompleteResult::Success)
	{
		StopAllSessionFindings();

		CurrentJoinedSessionName = SessionName;

		UE_LOG(LogTemp, Warning, TEXT("Joining Session: %s Successful, port: %d"), *(SessionName.ToString()), Port);

		FString TravelURL = "";
		SessionPtr->GetResolvedConnectString(SessionName, TravelURL);

		FString TestingURL = UCNetStatics::GetTestingURL();
		if (!TestingURL.IsEmpty())
		{
			TravelURL = TestingURL;
		}
		UCNetStatics::ReplacePort(TravelURL, Port);

		UE_LOG(LogTemp, Warning, TEXT("Traveling to Session at : %s"), *TravelURL);

		GetFirstLocalPlayerController(GetWorld())->ClientTravel(TravelURL, ETravelType::TRAVEL_Absolute);
	}
	else
	{
		OnJoinSessionFailed.Broadcast();
	}

	SessionPtr->OnJoinSessionCompleteDelegates.RemoveAll(this);
}

void UCGameInstance::SendCancelToCoordinator()
{
	if ( !PendingSessionSearchId.IsValid() )
		return;

	FHttpRequestRef Request = FHttpModule::Get().CreateRequest();
	FString URL = FString::Printf(TEXT("%s/Sessions/%s"), *UCNetStatics::GetCoordinatorURL(), *PendingSessionSearchId.ToString());

	Request->SetURL(URL);
	Request->SetVerb("DELETE");
	Request->ProcessRequest();

	UE_LOG(LogTemp, Warning, TEXT("Sent Cancel Request to Coordinator for Session SearchID: %s"), *PendingSessionSearchId.ToString());

	PendingSessionSearchId.Invalidate();
}

void UCGameInstance::OnLeaveSessionCompleted(FName InSessionName, bool bWasSuccessful)
{
	if ( IOnlineSessionPtr SessionPtr = UCNetStatics::GetSessionPtr() )
	{
		SessionPtr->OnDestroySessionCompleteDelegates.RemoveAll(this);
	}

	CurrentJoinedSessionName = NAME_None;

	const FName LevelURL = GetMainMenuURL();

	UE_LOG(LogTemp, Warning, TEXT("OnLeaveSessionCompleted - MainMenuLevel Raw: %s, LevelURL: %s"), *MainMenuLevel.ToString(), *LevelURL.ToString());

	if ( LevelURL != NAME_None )
	{
		UGameplayStatics::OpenLevel(this, LevelURL);
		OnReturnToMainMenu.Broadcast();
	}
}

void UCGameInstance::PlayerJoined(const FUniqueNetIdRepl& UniqueId, APlayerState* NewPlayerState)
{
	bWaitingForPlayer = false;

	if (WaitPlayerJoinTimeoutHandle.IsValid())
	{
		GetWorld()->GetTimerManager().ClearTimer(WaitPlayerJoinTimeoutHandle);
	}

	bool bUsesDesignatedHost = !DesignatedHostUniqueIdStr.IsEmpty();
	bool bShouldBeHost = bUsesDesignatedHost ? (UniqueId.IsValid() && UniqueId.GetUniqueNetId().IsValid() && UniqueId.GetUniqueNetId()->ToString() == DesignatedHostUniqueIdStr) : PlayerRecord.IsEmpty();

	if ( bShouldBeHost && HostPlayerId != UniqueId )
	{
		FUniqueNetIdRepl PreviousHostId = HostPlayerId;
		HostPlayerId = UniqueId;

		UpdateHostPlayerController(UniqueId, PreviousHostId);

		if(UWorld* World = GetWorld() )
		{
			if ( ACGameState* CGameState = World->GetGameState<ACGameState>() )
			{
				if ( bUsesDesignatedHost && !DesignatedHostNickname.IsEmpty() )
				{
					CGameState->SetHostSelection(UniqueId, DesignatedHostNickname);
				}
				else
				{
					ApplyHostNickname(CGameState, UniqueId, NewPlayerState);
				}
			}
		}

		DesignatedHostUniqueIdStr.Empty();
	}

	PlayerRecord.Add(UniqueId);

	UpdateAdvertisedPlayerCount();
}

void UCGameInstance::PlayerLeft(const FUniqueNetIdRepl& UniqueId)
{
	PlayerRecord.Remove(UniqueId);

	UpdateAdvertisedPlayerCount();

	if (PlayerRecord.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("All Player Left the session, Terminating"));
		TerminateSessionServer();
	}
}

void UCGameInstance::OnWorldChanged(UWorld* OldWorld, UWorld* NewWorld)
{
	Super::OnWorldChanged(OldWorld, NewWorld);

	if (NewWorld && bWaitingForPlayer)
	{
		NewWorld->GetTimerManager().SetTimer(WaitPlayerJoinTimeoutHandle, this, &UCGameInstance::WaitPlayerJoinTimeoutReached, WaitPlayerJoinTimeoutDuration);
	}
}

void UCGameInstance::UpdateAdvertisedHostNickname(const FString& NewHostNickname)
{
	IOnlineSessionPtr SessionPtr = UCNetStatics::GetSessionPtr();
	if ( !SessionPtr )
		return;

	FNamedOnlineSession* Session = SessionPtr->GetNamedSession(FName(ServerSessionName));
	if ( !Session )
		return;

	Session->SessionSettings.Set(UCNetStatics::GetHostNicknameKey(), NewHostNickname, EOnlineDataAdvertisementType::ViaOnlineService);
	SessionPtr->UpdateSession(FName(ServerSessionName), Session->SessionSettings, true);
}

void UCGameInstance::UpdateAdvertisedPlayerCount()
{
	IOnlineSessionPtr SessionPtr = UCNetStatics::GetSessionPtr();
	if ( !SessionPtr )
		return;

	FNamedOnlineSession* Session = SessionPtr->GetNamedSession(FName(ServerSessionName));
	if ( !Session )
		return;

	Session->SessionSettings.Set(UCNetStatics::GetPlayerCountKey(), PlayerRecord.Num(), EOnlineDataAdvertisementType::ViaOnlineService);

	SessionPtr->OnUpdateSessionCompleteDelegates.RemoveAll(this);
	SessionPtr->OnUpdateSessionCompleteDelegates.AddUObject(this, &UCGameInstance::OnPlayerCountUpdateCompleted);

	SessionPtr->UpdateSession(FName(ServerSessionName), Session->SessionSettings, true);
}

void UCGameInstance::UpdateHostPlayerController(const FUniqueNetIdRepl& NewHostId, const FUniqueNetIdRepl& PreviousHostId)
{
	UWorld* World = GetWorld();
	if ( !World )
		return;

	for ( FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It )
	{
		ALobbyPlayerController* PC = Cast<ALobbyPlayerController>(*It);
		APlayerState* PS = PC ? PC->GetPlayerState<APlayerState>() : nullptr;

		if ( !PC || !PS )
			continue;

		FUniqueNetIdRepl Id = PS->GetUniqueId();

		if ( Id == NewHostId )
			PC->SetbIsHost(true);
		else if ( Id == PreviousHostId )
			PC->SetbIsHost(false);
	}
}

void UCGameInstance::SetSessionAdvertised(bool bShouldAdvertise)
{
	IOnlineSessionPtr SessionPtr = UCNetStatics::GetSessionPtr();

	if ( !SessionPtr )
		return;

	FNamedOnlineSession* Session = SessionPtr->GetNamedSession(FName(ServerSessionName));
	if ( !Session )
		return;

	UE_LOG(LogTemp, Warning, TEXT("Set Session Advertised : %s"), bShouldAdvertise ? TEXT("true") : TEXT("false"));

	Session->SessionSettings.bShouldAdvertise = bShouldAdvertise;

	SessionPtr->OnUpdateSessionCompleteDelegates.RemoveAll(this);
	SessionPtr->OnUpdateSessionCompleteDelegates.AddUObject(this, &UCGameInstance::OnSessionAdvertisedUpdateCompleted);

	SessionPtr->UpdateSession(FName(ServerSessionName), Session->SessionSettings, true);
}

void UCGameInstance::OnSessionAdvertisedUpdateCompleted(FName SessionName, bool bWasSuccessful)
{
	UE_LOG(LogTemp, Warning, TEXT("SetSessionAdvertised UpdateSession Completed - Success: %s"), bWasSuccessful ? TEXT("true") : TEXT("false"));

	if ( IOnlineSessionPtr SessionPtr = UCNetStatics::GetSessionPtr() )
	{
		SessionPtr->OnUpdateSessionCompleteDelegates.RemoveAll(this);
	}
}

void UCGameInstance::UpdateAdvertisedHeroSelectionState(bool bStarted)
{
	IOnlineSessionPtr SessionPtr = UCNetStatics::GetSessionPtr();
	if ( !SessionPtr )
		return;

	FNamedOnlineSession* Session = SessionPtr->GetNamedSession(FName(ServerSessionName));
	if ( !Session )
		return;

	Session->SessionSettings.Set(UCNetStatics::GetHeroSelectionStartedKey(), bStarted ? 1 : 0, EOnlineDataAdvertisementType::ViaOnlineService);
	SessionPtr->UpdateSession(FName(ServerSessionName), Session->SessionSettings, true);
}

void UCGameInstance::CreateSession()
{
	IOnlineSessionPtr SessionPtr = UCNetStatics::GetSessionPtr();
	if (SessionPtr)
	{
		ServerSessionName = UCNetStatics::GetSessionNameStr();
		FString SessionSearchId = UCNetStatics::GetSessionSearchIdStr();
		ServerSessionPort = UCNetStatics::GetSessionPort();
		UE_LOG(LogTemp, Warning, TEXT("#### Create Session With Name : %s, ID : %s, Port : %d"), *ServerSessionName, *SessionSearchId, ServerSessionPort);

		FOnlineSessionSettings OnlineSessionSetting = UCNetStatics::GenerateOnlineSessionSettings(FName(ServerSessionName), *SessionSearchId, ServerSessionPort);

		FString RawHostNickname = TEXT("");
		FParse::Value(FCommandLine::Get(), TEXT("-HOST_NICKNAME="), RawHostNickname);
		RawHostNickname = RawHostNickname.TrimQuotes();
		FString HostNickname = FGenericPlatformHttp::UrlDecode(RawHostNickname);

		DesignatedHostNickname = HostNickname;

		OnlineSessionSetting.Set(UCNetStatics::GetHostNicknameKey(), HostNickname, EOnlineDataAdvertisementType::ViaOnlineService);
		OnlineSessionSetting.Set(UCNetStatics::GetPlayerCountKey(), 0, EOnlineDataAdvertisementType::ViaOnlineService);
		OnlineSessionSetting.Set(UCNetStatics::GetHeroSelectionStartedKey(), (int32)0, EOnlineDataAdvertisementType::ViaOnlineService);

		SessionPtr->OnCreateSessionCompleteDelegates.RemoveAll(this);
		SessionPtr->OnCreateSessionCompleteDelegates.AddUObject(this, &UCGameInstance::OnSessionCreated);
		if (!SessionPtr->CreateSession(0, FName(ServerSessionName), OnlineSessionSetting))
		{
			UE_LOG(LogTemp, Warning, TEXT("Session Creating Failed Right Away !!!!"));
			SessionPtr->OnCreateSessionCompleteDelegates.RemoveAll(this);
			TerminateSessionServer();
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Can't find Session Ptr, Terminating"));
		TerminateSessionServer();
	}
}

void UCGameInstance::OnSessionCreated(FName SessionName, bool bWasSuccessful)
{
	if (bWasSuccessful)
	{
		bWaitingForPlayer = true;

		UE_LOG(LogTemp, Warning, TEXT("--------------------- Session Created !!!! ---------------------"));
		GetWorld()->GetTimerManager().SetTimer(WaitPlayerJoinTimeoutHandle, this, &UCGameInstance::WaitPlayerJoinTimeoutReached, WaitPlayerJoinTimeoutDuration);

		UE_LOG(LogTemp, Warning, TEXT("Timer Valid after SetTimer: %s"),
			WaitPlayerJoinTimeoutHandle.IsValid() ? TEXT("true") : TEXT("false"));

		LoadLevelAndListen(LobbyLevel);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("--------------------- Session Creation Failed !!!! ---------------------"))
		TerminateSessionServer();
	}

	if (IOnlineSessionPtr SessionPtr = UCNetStatics::GetSessionPtr())
	{
		SessionPtr->OnCreateSessionCompleteDelegates.RemoveAll(this);

	}
}

void UCGameInstance::EndSessionCompleted(FName SessionName, bool bWasSuccessful)
{
	FGenericPlatformMisc::RequestExit(false);
}

void UCGameInstance::ApplyHostNickname(ACGameState* CGameState, const FUniqueNetIdRepl& HostId, APlayerState* HostPlayerStatePtr, int32 RetryCount)
{
	if ( !CGameState )
		return;

	FString Nickname = HostPlayerStatePtr ? HostPlayerStatePtr->GetPlayerName() : FString();

	CGameState->SetHostSelection(HostId, Nickname);

	if ( Nickname.IsEmpty() && HostPlayerStatePtr && RetryCount < 5 )
	{
		UWorld* World = GetWorld();
		if ( !World )
			return;

		TWeakObjectPtr<UCGameInstance> WeakThis(this);
		TWeakObjectPtr<ACGameState> WeakGS(CGameState);
		TWeakObjectPtr<APlayerState> WeakPS(HostPlayerStatePtr);
		FUniqueNetIdRepl CapturedId = HostId;
		int32 NextRetryCount = RetryCount + 1;

		FTimerHandle RetryHandle;
		World->GetTimerManager().SetTimer(RetryHandle, FTimerDelegate::CreateLambda(
			[ WeakThis, WeakGS, WeakPS, CapturedId, NextRetryCount ] ()
			{
				if ( WeakThis.IsValid() && WeakGS.IsValid() && WeakPS.IsValid() )
				{
					WeakThis->ApplyHostNickname(WeakGS.Get(), CapturedId, WeakPS.Get(), NextRetryCount);
				}
			}), 0.5f, false);
	}
	else if ( Nickname.IsEmpty() && RetryCount >= 5 )
	{
		UE_LOG(LogTemp, Warning, TEXT("ApplyHostNickname: PlayerName still empty after 5 retries, giving up."));
	}
}

void UCGameInstance::OnPlayerCountUpdateCompleted(FName SessionName, bool bWasSuccessful)
{
	UE_LOG(LogTemp, Warning, TEXT("UpdateSession (PlayerCount) Completed - Success: %s"), bWasSuccessful ? TEXT("true") : TEXT("false"));

	if ( IOnlineSessionPtr SessionPtr = UCNetStatics::GetSessionPtr() )
	{
		SessionPtr->OnUpdateSessionCompleteDelegates.RemoveAll(this);
	}
}

void UCGameInstance::TerminateSessionServer()
{
	if (IOnlineSessionPtr SessionPtr = UCNetStatics::GetSessionPtr())
	{
		SessionPtr->OnEndSessionCompleteDelegates.RemoveAll(this);
		SessionPtr->OnEndSessionCompleteDelegates.AddUObject(this, &UCGameInstance::EndSessionCompleted);

		if (!SessionPtr->EndSession(FName{ ServerSessionName }))
		{
			FGenericPlatformMisc::RequestExit(false);
		}
	}
	else
	{
		FGenericPlatformMisc::RequestExit(false);
	}
}

void UCGameInstance::WaitPlayerJoinTimeoutReached()
{
	UE_LOG(LogTemp, Warning, TEXT("Session Server shut down after %f seconds without player join"), WaitPlayerJoinTimeoutDuration);

	TerminateSessionServer(); 
}

void UCGameInstance::LoadLevelAndListen(TSoftObjectPtr<UWorld> Level)
{
	const FName LevelURL = FName(*FPackageName::ObjectPathToPackageName(Level.ToString()));

	if (LevelURL != "")
	{
		FString TravelStr = FString::Printf(TEXT("%s?listen?port=%d"), *LevelURL.ToString(), ServerSessionPort);
		UE_LOG(LogTemp, Warning, TEXT("Server Traveling to : %s"), *(TravelStr));
		GetWorld()->ServerTravel(TravelStr);
	}
}
