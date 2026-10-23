// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineBaseTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Player/PlayerInfoTypes.h"
#include "CLoadingScreenSubsystem.generated.h"

class UMatchLoadingWidget;

/**
 * 
 */
UCLASS()
class UCLoadingScreenSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	
	void ShowLoadingScreen();
	void HideLoadingScreen();
	void RequestHideLoadingScreen();
	bool IsLoadingScreenVisible() const { return LoadingWidget != nullptr; }
	static bool ShouldSkipMatchLoading(const UObject* WorldContextObject);

	const TArray<FPlayerSelection>& GetRosterSnapshot() const { return RosterSnapshot; }

	bool IsInGameLevel(const UWorld* World) const;
	bool IsLocalPlayerSelection(const FPlayerSelection& Selection) const;

	float GetGameLevelLoadProgress() const;

private:
	void HandleSeamlessTravelStart(UWorld* CurrentWorld, const FString& LevelName);
	void HandleNetworkFailure(UWorld* World, class UNetDriver* NetDriver, ENetworkFailure::Type FailureType, const FString& ErrorString);
	void HandleTravelFailure(UWorld* World, ETravelFailure::Type FailureType, const FString& ErrorString);

	bool IsGameLevelName(const FString& LevelName) const;
	void CaptureRoster(UWorld* LobbyWorld);

	UPROPERTY()
	UMatchLoadingWidget* LoadingWidget = nullptr;

	TSharedPtr<SWidget> LoadingSlateWidget;

	UPROPERTY()
	TArray<FPlayerSelection> RosterSnapshot;

	double TravelStartTime = 0.0;
	mutable float CachedMapProgress = 0.f;

	FDelegateHandle SeamlessTravelStartHandle;
	FDelegateHandle NetworkFailureHandle;
	FDelegateHandle TravelFailureHandle;

	static constexpr int32 LoadingScreenZOrder = 10000;
};
