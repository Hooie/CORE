// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "Player/PlayerInfoTypes.h"
#include "Player/KillFeedTypes.h"
#include "CGameState.generated.h"

class UPA_CharacterDefinition;

UENUM()
enum class EMatchPhase : uint8
{
	WaitingForPlayers,
	Countdown,
	InProgress,
	Finished
};

DECLARE_MULTICAST_DELEGATE_OneParam(FOnMatchPhaseChanged, EMatchPhase /*NewPhase*/);

DECLARE_MULTICAST_DELEGATE_OneParam(FOnPlayerSelectionUpdated, const TArray<FPlayerSelection>& /*NewPlayerSelection*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnHeroSelectionTimeOut, bool /*CanForceStartMatch*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnHeroSelectionTimerUpdated, int32 /*RemainingTime*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnHostNicknameChanged, const FString& /*NewHostNickname*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnKillFeedEntryReceived, const FKillFeedEntry& /*KillFeedEntry*/);

/**
 * 
 */
UCLASS()
class ACGameState : public AGameStateBase
{
	GENERATED_BODY()
	
public:
	virtual void RemovePlayerState(APlayerState* PlayerState) override;
	virtual void BeginPlay() override;

	void RequestPlayerSelectionChange(const APlayerState* RequestingPlayer, uint8 DesiredSlot);
	void SetCharacterSelection(const APlayerState* SelectingPlayer, const UPA_CharacterDefinition* SelectedDefinition);
	bool IsSlotOccupied(uint8 SlotId) const;
	bool IsDefinitionSelected(const UPA_CharacterDefinition* Definition) const;
	bool IsDefinitionSelectedByTeam(const UPA_CharacterDefinition* Definition, uint8 PlayerSlot) const;

	void SetCharacterDeselected(const UPA_CharacterDefinition* DefinitionToDeselect);
	void SetCharacterDeselectedForPlayer(const APlayerState* SelectingPlayer);

	FOnPlayerSelectionUpdated OnPlayerSelectionUpdated;
	FOnHeroSelectionTimeOut OnHeroSelectionTimeOut;
	FOnHeroSelectionTimerUpdated OnHeroSelectionTimerUpdated;
	FOnHostNicknameChanged OnHostNicknameChanged;
	FOnKillFeedEntryReceived OnKillFeedEntryReceived;

	FString GetHostNickname() const { return HostNickname; }

	virtual void GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const override;

	const TArray<FPlayerSelection>& GetPlayerSelection() const;

	bool CanStartHeroSelection() const;
	bool CanStartMatch() const;
	bool CanForceStartMatch() const;

	void SetPlayerReady(const APlayerState* RequestingPlayer, bool bReady);
	bool GetIsPlayerReady(const APlayerState* RequestingPlayer);
	void ResetAllPlayersReady();
	void ResetAllCharacterSelections();

	void StartHeroSelectionTimer();

	void RemovePlayerSelection(const APlayerState* LeavingPlayer);
	const FPlayerSelection* FindNextHost(const APlayerState* ExcludePlayer) const;
	void SetHostSelection(const FUniqueNetIdRepl& NewHostId, const FString& NewHostNickname);

	bool IsHeroSelectionStarted() const { return bHeroSelectionStarted; }

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_BroadcastKillFeed(const FKillFeedEntry& Entry);

	float GetMatchElapsedSeconds() const;

	void RegisterFirstWaveSpawnTime();
	float GetTimeSinceFirstWave() const;

	void SetFirstWavePlannedTime(float DelayFromNow);
	float GetTimeUntilFirstWave() const;
	bool HasFirstWaveSpawned() const { return FirstWaveSpawnServerTime >= 0.f; }

	bool GetBaseLocationForTeam(uint8 TeamId, FVector& OutLocation, FRotator& OutRotation) const;

	FOnMatchPhaseChanged OnMatchPhaseChanged;

	void SetMatchPhase(EMatchPhase NewPhase, float CountdownDuration = 0.f, float AnnounceDuration = 0.f);
	EMatchPhase GetMatchPhase() const { return MatchPhase; }
	float GetCountdownRemaining() const;
	float GetCountdownNumberDuration() const { return CountdownNumberDuration; }

	static bool IsMatchInProgress(const UObject* WorldContextObject);

private:
	UPROPERTY(ReplicatedUsing = OnRep_PlayerSelectionArray)
	TArray<FPlayerSelection> PlayerSelectionArray;

	UFUNCTION()
	void OnRep_PlayerSelectionArray();

	UPROPERTY(ReplicatedUsing = OnRep_HeroSelectionReaminingTime)
	int32 HeroSelectionRemainingTime = 0;

	UFUNCTION()
	void OnRep_HeroSelectionReaminingTime();

	FTimerHandle HeroSelectionTimerHandle;

	void UpdateHeroSelectionTimer();

	UPROPERTY(ReplicatedUsing = OnRep_bHeroSelectionTimeOut)
	bool bHeroSelectionTimeOut = false;

	UFUNCTION()
	void OnRep_bHeroSelectionTimeOut();

	UPROPERTY(ReplicatedUsing = OnRep_HostNickname)
	FString HostNickname;

	UFUNCTION()
	void OnRep_HostNickname();

	bool bHeroSelectionStarted = false;

	UPROPERTY(Replicated)
	float MatchStartServerTime = -1.f;

	UPROPERTY(Replicated)
	float FirstWaveSpawnServerTime = -1.f;

	UPROPERTY(ReplicatedUsing = OnRep_MatchPhase)
	EMatchPhase MatchPhase = EMatchPhase::WaitingForPlayers;

	UFUNCTION()
	void OnRep_MatchPhase();

	UPROPERTY(Replicated)
	float CountdownEndServerTime = -1.f;

	UPROPERTY(Replicated)
	float CountdownNumberDuration = 0.f;

	UPROPERTY(Replicated)
	float FirstWavePlannedServerTime = -1.f;
};
