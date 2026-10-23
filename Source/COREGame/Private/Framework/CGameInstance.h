// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Interfaces/IHttpResponse.h"
#include "Interfaces/IHttpRequest.h"
#include "OnlineSessionSettings.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "CGameInstance.generated.h"

DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnLoginCompleted, bool /*bWasSuccessful*/, const FString& /*PlayerNickname*/, const FString& /*ErrorMsg*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnLogoutCompleted, bool /*bWasSuccessful*/);
DECLARE_MULTICAST_DELEGATE(FOnJoinSessionFailed);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnGlobalSessionSearchCompleted, const TArray<FOnlineSessionSearchResult>& /*SearchResults*/);
DECLARE_MULTICAST_DELEGATE(FOnReturnToMainMenu);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnJoinRejected, const FString& /*RejectReason*/);

/**
 * 
 */
UCLASS()
class UCGameInstance : public UGameInstance
{
	GENERATED_BODY()
	
public:
	void StartMatch();

	// 게임 맵에서 기다릴 인원의 수 (한번 읽으면 0으로 초기화)
	int32 ConsumeExpectedMatchPlayerCount();

	TSubclassOf<class UMatchLoadingWidget> GetMatchLoadingWidgetClass() const { return MatchLoadingWidgetClass; }
	FString GetGameLevelPackageName() const;
	FString GetGameLevelShortName() const;

	virtual void Init() override;
	virtual void ReturnToMainMenu() override;
	virtual void Shutdown() override;

	const FName GetMainMenuURL();

private:
	int32 ExpectedMatchPlayerCount = 0;

/*************************************/
/*               login               */
/*************************************/
public:
	FOnLoginCompleted OnLoginCompleted;
	FOnLogoutCompleted OnLogoutCompleted;

	bool IsLoggedIn() const;
	bool IsLoggingIn() const;
	void ClientAccountPortalLogin();

	void Logout();

	FString GetCachedPlayerNickname() const { return CachedPlayerNickname; }

protected:
	virtual void OnStart() override;

private:
	void OnPostLoadMap(UWorld* LoadedWorld);

	void ClientLogin(const FString& Type, const FString& Id, const FString& Token);
	void LoginCompleted(int NumOfLocalPlayer, bool bWasSuccessful, const FUniqueNetId& UserId, const FString& Error);
	
	void LogoutCompleted(int32 LocalUserNum, bool bWasSuccessful);

	FDelegateHandle LoggingInDelegateHandle;
	FDelegateHandle LoggingOutDelegateHandle;

	FString CachedPlayerNickname;

/**************************************/
/* Client Session Creation And Search */
/**************************************/
public:
	void RequestCreateAndJoinSession(const FName& NewSessionName);
	void CancelSessionCreation();
	void StartGlobalSessionSearch();
	bool JoinSessionWithId(const FString& SessionIdStr);
	void RefreshSessionsNow();

	FOnJoinSessionFailed OnJoinSessionFailed;
	FOnGlobalSessionSearchCompleted OnGlobalSessionSearchCompleted;
	FOnReturnToMainMenu OnReturnToMainMenu;
	FOnJoinRejected OnJoinRejected;

	FString ConsumePendingJoinRejectReason();

	void LeaveCurrentSessionAndReturnToMainMenu();

	FName GetCurrentJoinedSessionName() const { return CurrentJoinedSessionName; }

private:
	void SessionCreationRequestCompleted(FHttpRequestPtr Request, FHttpResponsePtr Response, bool bConnectedSuccessfully, FGuid SessionSearchId);
	void StartFindingCreatedSession(const FGuid& SessionSearchId);
	void StopAllSessionFindings();
	void StopFindingCreatedSession();
	void StopGlobalSessionSearch();
	void FindGlobalSessions();
	void GlobalSessionSearchCompleted(bool bWasSuccessful);
	void HandleNetworkFailure(UWorld* World, UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString);
	FTimerHandle FindCreatedSessionSearchTimerHandle;
	FTimerHandle FindCreatedSessionTimeoutTimerHandle;

	FTimerHandle GlobalSessionSearchTimerHandle;

	UPROPERTY(EditDefaultsOnly, Category = "Session Search")
	float GlobalSessionSearchInterval = 2.f;

	UPROPERTY(EditDefaultsOnly, Category = "Session Search")
	float FindCreatedSessionSearchInterval = 1.f;

	UPROPERTY(EditDefaultsOnly, Category = "Session Search")
	float FindCreatedSessionTimeoutDuration = 60.f;

	void FindCreatedSession(FGuid SessionSearchId);
	void FindCreatedSessionTimeout();
	void FindCreateSessionCompleted(bool bWasSuccessful);
	void JoinSessionWithSearchResult(const class FOnlineSessionSearchResult& SearchResult);
	void JoinSessionCompleted(FName SessionName, EOnJoinSessionCompleteResult::Type JoinResult, int Port);

	TSharedPtr<class FOnlineSessionSearch> SessionSearch;

	void SendCancelToCoordinator();

	FHttpRequestPtr PendingSessionRequest;
	FGuid PendingSessionSearchId;

	void OnLeaveSessionCompleted(FName InSessionName, bool bWasSuccessful);
	FName CurrentJoinedSessionName;

	FString PendingJoinRejectReason;

/*************************************/
/*           Session Server          */
/*************************************/
public:
	void PlayerJoined(const FUniqueNetIdRepl& UniqueId, APlayerState* NewPlayerState);
	void PlayerLeft(const FUniqueNetIdRepl& UniqueId);
	virtual void OnWorldChanged(UWorld* OldWorld, UWorld* NewWorld) override;

	FUniqueNetIdRepl GetHostPlayerId() const { return HostPlayerId; }

	void TransferHost(const FUniqueNetIdRepl& NewHostId) { HostPlayerId = NewHostId; }

	void UpdateAdvertisedHostNickname(const FString& NewHostNickname);
	void UpdateAdvertisedPlayerCount();
	void UpdateHostPlayerController(const FUniqueNetIdRepl& NewHostId, const FUniqueNetIdRepl& PreviousHostId);
	void SetSessionAdvertised(bool bShouldAdvertise);
	void OnSessionAdvertisedUpdateCompleted(FName SessionName, bool bWasSuccessful);
	void UpdateAdvertisedHeroSelectionState(bool bStarted);

private:
	void CreateSession();
	void OnSessionCreated(FName SessionName, bool bWasSuccessful);
	void EndSessionCompleted(FName SessionName, bool bWasSuccessful);

	void ApplyHostNickname(class ACGameState* CGameState, const FUniqueNetIdRepl& HostId, APlayerState* HostPlayerStatePtr, int32 RetryCount = 0);

	void OnPlayerCountUpdateCompleted(FName SessionName, bool bWasSuccessful);

	FString ServerSessionName;
	int ServerSessionPort;
		
	FString DesignatedHostUniqueIdStr;
	FString DesignatedHostNickname;

	void TerminateSessionServer();

	FTimerHandle WaitPlayerJoinTimeoutHandle;
	bool bWaitingForPlayer = false;

	UPROPERTY(EditDefaultsOnly, Category = "Session")
	float WaitPlayerJoinTimeoutDuration = 60.f;

	void WaitPlayerJoinTimeoutReached();

	TSet<FUniqueNetIdRepl> PlayerRecord;
	FUniqueNetIdRepl HostPlayerId;

private:
	UPROPERTY(EditDefaultsOnly, Category = "Map")
	TSoftObjectPtr<UWorld> MainMenuLevel;

	UPROPERTY(EditDefaultsOnly, Category = "Map")
	TSoftObjectPtr<UWorld> LobbyLevel;

	UPROPERTY(EditDefaultsOnly, Category = "Map")
	TSoftObjectPtr<UWorld> GameLevel;

	UPROPERTY(EditDefaultsOnly, Category = "UI")
	TSubclassOf<class UMatchLoadingWidget> MatchLoadingWidgetClass;

	void LoadLevelAndListen(TSoftObjectPtr<UWorld> Level);
};
