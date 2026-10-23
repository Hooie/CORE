// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/CLoadingScreenSubsystem.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Framework/CGameInstance.h"
#include "Framework/CGameState.h"
#include "HAL/IConsoleManager.h"
#include "Misc/PackageName.h"
#include "UObject/UObjectGlobals.h"
#include "Widgets/MatchLoadingWidget.h"

static TAutoConsoleVariable<bool> CVarSkipMatchLoadingInPIE(
	TEXT("Cruch.SkipMatchLoadingInPIE"),
	true,
	TEXT("에디터(PIE)에서 매치 로딩 화면과 플레이어 대기, 카운트다운을 건너뜁니다."));

bool UCLoadingScreenSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	return !IsRunningDedicatedServer() && Super::ShouldCreateSubsystem(Outer);
}

void UCLoadingScreenSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	SeamlessTravelStartHandle = FWorldDelegates::OnSeamlessTravelStart.AddUObject(this, &UCLoadingScreenSubsystem::HandleSeamlessTravelStart);

	if ( GEngine )
	{
		NetworkFailureHandle = GEngine->OnNetworkFailure().AddUObject(this, &UCLoadingScreenSubsystem::HandleNetworkFailure);
		TravelFailureHandle = GEngine->OnTravelFailure().AddUObject(this, &UCLoadingScreenSubsystem::HandleTravelFailure);
	}
}

void UCLoadingScreenSubsystem::Deinitialize()
{
	FWorldDelegates::OnSeamlessTravelStart.Remove(SeamlessTravelStartHandle);

	if ( GEngine )
	{
		GEngine->OnNetworkFailure().Remove(NetworkFailureHandle);
		GEngine->OnTravelFailure().Remove(TravelFailureHandle);
	}

	HideLoadingScreen();

	Super::Deinitialize();
}

void UCLoadingScreenSubsystem::ShowLoadingScreen()
{
	if ( ShouldSkipMatchLoading(GetGameInstance()) )
		return;

	if ( LoadingWidget )
		return;

	UCGameInstance* CGameInstance = Cast<UCGameInstance>(GetGameInstance());
	UGameViewportClient* ViewportClient = CGameInstance ? CGameInstance->GetGameViewportClient() : nullptr;

	if ( !ViewportClient || !CGameInstance->GetMatchLoadingWidgetClass() )
		return;

	if ( TravelStartTime <= 0.0 )
	{
		TravelStartTime = FPlatformTime::Seconds();
	}

	LoadingWidget = CreateWidget<UMatchLoadingWidget>(CGameInstance, CGameInstance->GetMatchLoadingWidgetClass());
	if ( !LoadingWidget )
		return;

	LoadingSlateWidget = LoadingWidget->TakeWidget();
	ViewportClient->AddViewportWidgetContent(LoadingSlateWidget.ToSharedRef(), LoadingScreenZOrder);
}

void UCLoadingScreenSubsystem::HideLoadingScreen()
{
	if ( LoadingSlateWidget.IsValid() )
	{
		if ( UGameInstance* GameInstance = GetGameInstance() )
		{
			if ( UGameViewportClient* ViewportClient = GameInstance->GetGameViewportClient())
			{
				ViewportClient->RemoveViewportWidgetContent(LoadingSlateWidget.ToSharedRef());
			}
		}
	}

	LoadingSlateWidget.Reset();
	LoadingWidget = nullptr;
	RosterSnapshot.Reset();
	TravelStartTime = 0.0;
	CachedMapProgress = 0.f;
}

void UCLoadingScreenSubsystem::RequestHideLoadingScreen()
{
	if ( UGameInstance* GameInstance = GetGameInstance() )
	{
		GameInstance->GetTimerManager().SetTimerForNextTick(this, &UCLoadingScreenSubsystem::HideLoadingScreen);
	}
}

bool UCLoadingScreenSubsystem::ShouldSkipMatchLoading(const UObject* WorldContextObject)
{
#if WITH_EDITOR
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	return World && World->WorldType == EWorldType::PIE && CVarSkipMatchLoadingInPIE.GetValueOnGameThread();
#else
	return false;
#endif
}

bool UCLoadingScreenSubsystem::IsInGameLevel(const UWorld* World) const
{
	return World && IsGameLevelName(UWorld::RemovePIEPrefix(World->GetMapName()));
}

bool UCLoadingScreenSubsystem::IsLocalPlayerSelection(const FPlayerSelection& Selection) const
{
	const UGameInstance* GameInstance = GetGameInstance();
	const ULocalPlayer* LocalPlayer = GameInstance ? GameInstance->GetFirstGamePlayer() : nullptr;

	if ( LocalPlayer )
	{
		const FUniqueNetIdRepl LocalId = LocalPlayer->GetPreferredUniqueNetId();
		if ( LocalId.IsValid() && Selection.GetPlayerUniqueId().IsValid() )
		{
			return LocalId == Selection.GetPlayerUniqueId();
		}
	}

	if ( const UCGameInstance* CGameInstance = Cast<UCGameInstance>(GetGameInstance()) )
	{
		const FString LocalNickName = CGameInstance->GetCachedPlayerNickname();
		return !LocalNickName.IsEmpty() && LocalNickName == Selection.GetPlayerNickName();
	}

	return false;
}

float UCLoadingScreenSubsystem::GetGameLevelLoadProgress() const
{
	float Progress = -1.f;

	if ( const UCGameInstance* CGameInstance = Cast<UCGameInstance>(GetGameInstance()) )
	{
		const FString PackageName = CGameInstance->GetGameLevelPackageName();
		if ( !PackageName.IsEmpty() )
		{
			const float Percent = GetAsyncLoadPercentage(FName(*PackageName));
			if ( Percent >= 0.f )
			{
				Progress = Percent / 100.f;
			}
		}
	}

	if ( Progress < 0.f )
	{
		const float Elapsed = TravelStartTime > 0.0 ? ( float ) (FPlatformTime::Seconds() - TravelStartTime) : 0.f;
		Progress = 0.9f * (1.f - FMath::Exp(-Elapsed / 4.f));
	}

	CachedMapProgress = FMath::Max(CachedMapProgress, FMath::Clamp(Progress, 0.f, 1.f));
	return CachedMapProgress;
}

void UCLoadingScreenSubsystem::HandleSeamlessTravelStart(UWorld* CurrentWorld, const FString& LevelName)
{
	if ( !CurrentWorld || CurrentWorld->GetGameInstance() != GetGameInstance() )
		return;

	if ( !IsGameLevelName(LevelName) )
		return;

	CaptureRoster(CurrentWorld);

	TravelStartTime = FPlatformTime::Seconds();
	CachedMapProgress = 0.f;

	ShowLoadingScreen();
}

void UCLoadingScreenSubsystem::HandleNetworkFailure(UWorld* World, class UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString)
{
	if ( World && World->GetGameInstance() != GetGameInstance() )
		return;

	HideLoadingScreen();
}

void UCLoadingScreenSubsystem::HandleTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& ErrorString)
{
	if ( World && World->GetGameInstance() != GetGameInstance() )
		return;

	HideLoadingScreen();
}

bool UCLoadingScreenSubsystem::IsGameLevelName(const FString& LevelName) const
{
	const UCGameInstance* CGameInstance = Cast<UCGameInstance>(GetGameInstance());
	if ( !CGameInstance )
		return false;

	const FString GameLevelName = CGameInstance->GetGameLevelShortName();
	return !GameLevelName.IsEmpty() && FPackageName::GetShortName(LevelName).Equals(GameLevelName, ESearchCase::IgnoreCase);
}

void UCLoadingScreenSubsystem::CaptureRoster(UWorld* LobbyWorld)
{
	RosterSnapshot.Reset();

	if ( const ACGameState* LobbyGameState = LobbyWorld ? LobbyWorld->GetGameState<ACGameState>() : nullptr )
	{
		RosterSnapshot = LobbyGameState->GetPlayerSelection();
	}
}

