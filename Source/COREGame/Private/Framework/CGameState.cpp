// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/CGameState.h"
#include "EngineUtils.h"
#include "Framework/CGameInstance.h"
#include "Framework/BaseZone.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"
#include "Network/CNetStatics.h"
#include "Player/CPlayerState.h"

void ACGameState::RemovePlayerState(APlayerState* PlayerState)
{
	Super::RemovePlayerState(PlayerState);

	OnPlayerSelectionUpdated.Broadcast(PlayerSelectionArray);
}

void ACGameState::BeginPlay()
{
	Super::BeginPlay();
}

void ACGameState::RequestPlayerSelectionChange(const APlayerState* RequestingPlayer, uint8 DesiredSlot)
{
	if (!HasAuthority() || IsSlotOccupied(DesiredSlot))
		return;

	FPlayerSelection* PlayerSelectionPtr = PlayerSelectionArray.FindByPredicate(
		[ & ] (const FPlayerSelection& S) {
			return S.IsForPlayer(RequestingPlayer);
		}
	);

	if (PlayerSelectionPtr)
	{
		if ( !PlayerSelectionPtr->GetbIsHost() && PlayerSelectionPtr->GetbIsReady() )
			return;

		PlayerSelectionPtr->SetSlot(DesiredSlot);
	}
	else
	{
		FPlayerSelection NewSelection(DesiredSlot, RequestingPlayer);

		UCGameInstance* CGameInstance = GetGameInstance<UCGameInstance>();
		if ( CGameInstance )
		{
			bool bIsHost = CGameInstance->GetHostPlayerId() == RequestingPlayer->GetUniqueId();
			NewSelection.SetbIsHost(bIsHost);

			if ( bIsHost )
				NewSelection.SetbIsReady(true);
		}
		
		PlayerSelectionArray.Add(NewSelection);
	}

	OnPlayerSelectionUpdated.Broadcast(PlayerSelectionArray);
	ForceNetUpdate();
}

void ACGameState::SetCharacterSelection(const APlayerState* SelectingPlayer, const UPA_CharacterDefinition* SelectedDefinition)
{
	FPlayerSelection* FoundPlayerSelection = PlayerSelectionArray.FindByPredicate(
		[&](const FPlayerSelection& PlayerSelection)
		{
			return PlayerSelection.IsForPlayer(SelectingPlayer);
		}
	);

	if ( !FoundPlayerSelection )
		return;
			
	if ( IsDefinitionSelectedByTeam(SelectedDefinition, FoundPlayerSelection->GetPlayerSlot()) )
	{
		return;
	}
	
	FoundPlayerSelection->SetCharacterDefinition(SelectedDefinition);
	OnPlayerSelectionUpdated.Broadcast(PlayerSelectionArray);
	
}

bool ACGameState::IsSlotOccupied(uint8 SlotId) const
{
	for (const FPlayerSelection& PlayerSelection : PlayerSelectionArray)
	{
		if (PlayerSelection.GetPlayerSlot() == SlotId)
			return true;
	}

	return false;
}

bool ACGameState::IsDefinitionSelected(const UPA_CharacterDefinition* Definition) const
{
	const FPlayerSelection* FoundPlayerSelection = PlayerSelectionArray.FindByPredicate(
		[&](const FPlayerSelection& PlayerSelection)
		{
			return PlayerSelection.GetCharacterDefinition() == Definition;
		}
	);

	return FoundPlayerSelection != nullptr;
}

bool ACGameState::IsDefinitionSelectedByTeam(const UPA_CharacterDefinition* Definition, uint8 PlayerSlot) const
{
	int32 TeamSize = UCNetStatics::GetPlayerCountPerTeam();
	bool bIsTeamOne = PlayerSlot < TeamSize;

	const FPlayerSelection* FoundPlayerSelection = PlayerSelectionArray.FindByPredicate(
		[ & ] (const FPlayerSelection& PlayerSelection)
		{
			if ( PlayerSelection.GetCharacterDefinition() != Definition )
				return false;
			
			bool bOtherIsTeamOne = PlayerSelection.GetPlayerSlot() < TeamSize;
			
			return bIsTeamOne == bOtherIsTeamOne;
		}
	);

	return FoundPlayerSelection != nullptr;
}

void ACGameState::SetCharacterDeselected(const UPA_CharacterDefinition* DefinitionToDeselect)
{
	if (!DefinitionToDeselect)
		return;

	FPlayerSelection* FoundPlayerSelection = PlayerSelectionArray.FindByPredicate(
		[&](const FPlayerSelection& PlayerSelection)
		{
			return PlayerSelection.GetCharacterDefinition() == DefinitionToDeselect;
		}
	);

	if (FoundPlayerSelection)
	{
		FoundPlayerSelection->SetCharacterDefinition(nullptr);
		OnPlayerSelectionUpdated.Broadcast(PlayerSelectionArray);
	}
}

void ACGameState::SetCharacterDeselectedForPlayer(const APlayerState* SelectingPlayer)
{
	FPlayerSelection* FoundPlayerSelection = PlayerSelectionArray.FindByPredicate(
		[ & ] (const FPlayerSelection& PlayerSelection)
		{
			return PlayerSelection.IsForPlayer(SelectingPlayer);
		}
	);

	if ( FoundPlayerSelection )
	{
		FoundPlayerSelection->SetCharacterDefinition(nullptr);
		OnPlayerSelectionUpdated.Broadcast(PlayerSelectionArray);
	}
}

void ACGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(ACGameState, PlayerSelectionArray, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME(ACGameState, HeroSelectionRemainingTime);
	DOREPLIFETIME(ACGameState, bHeroSelectionTimeOut);
	DOREPLIFETIME(ACGameState, HostNickname);
	DOREPLIFETIME(ACGameState, MatchStartServerTime);
	DOREPLIFETIME(ACGameState, FirstWaveSpawnServerTime);
	DOREPLIFETIME(ACGameState, FirstWavePlannedServerTime);

	DOREPLIFETIME(ACGameState, MatchPhase);
	DOREPLIFETIME(ACGameState, CountdownEndServerTime);
	DOREPLIFETIME(ACGameState, CountdownNumberDuration);
}

const TArray<FPlayerSelection>& ACGameState::GetPlayerSelection() const
{
	return PlayerSelectionArray;
}

bool ACGameState::CanStartHeroSelection() const
{
	if ( PlayerSelectionArray.Num() != PlayerArray.Num() )
		return false;

	for ( const FPlayerSelection& Selection : PlayerSelectionArray )
	{
		if ( !Selection.GetbIsHost() && !Selection.GetbIsReady() )
			return false;
	}

	return true;
}

bool ACGameState::CanStartMatch() const
{
	if ( PlayerSelectionArray.Num() != PlayerArray.Num() )
		return false;

	for (const FPlayerSelection& PlayerSelection : PlayerSelectionArray)
	{
		if ( PlayerSelection.GetCharacterDefinition() == nullptr )
			return false;

		if ( !PlayerSelection.GetbIsHost() && !PlayerSelection.GetbIsReady() )
			return false;
	}

	return true;
}

bool ACGameState::CanForceStartMatch() const
{
	if ( PlayerSelectionArray.Num() != PlayerArray.Num() )
		return false;

	for ( const FPlayerSelection& PlayerSelection : PlayerSelectionArray )
	{
		if ( PlayerSelection.GetCharacterDefinition() == nullptr )
			return false;
	}

	return true;
}

void ACGameState::SetPlayerReady(const APlayerState* RequestingPlayer, bool bReady)
{
	if ( !HasAuthority() )
		return;

	FPlayerSelection* Selection = PlayerSelectionArray.FindByPredicate(
		[ & ] (const FPlayerSelection& S)
		{
			return S.IsForPlayer(RequestingPlayer);
		}
	);

	if ( Selection )
	{
		Selection->SetbIsReady(bReady);

		OnPlayerSelectionUpdated.Broadcast(PlayerSelectionArray);
		ForceNetUpdate();
	}
}

bool ACGameState::GetIsPlayerReady(const APlayerState* RequestingPlayer)
{
	FPlayerSelection* Selection = PlayerSelectionArray.FindByPredicate(
		[ & ] (const FPlayerSelection& S)
		{
			return S.IsForPlayer(RequestingPlayer);
		}
	);

	if ( Selection )
	{
		return Selection->GetbIsReady();
	}

	return false;
}

void ACGameState::ResetAllPlayersReady()
{
	if ( !HasAuthority() )
		return;

	for ( FPlayerSelection& Selection : PlayerSelectionArray )
	{
		Selection.SetbIsReady(false);
	}

	OnPlayerSelectionUpdated.Broadcast(PlayerSelectionArray);
	ForceNetUpdate();
}

void ACGameState::ResetAllCharacterSelections()
{
	if ( !HasAuthority() )
		return;

	for ( FPlayerSelection& Selection : PlayerSelectionArray )
	{
		Selection.SetCharacterDefinition(nullptr);
	}

	for ( APlayerState* PS : PlayerArray )
	{
		if ( ACPlayerState* CPlayerState = Cast<ACPlayerState>(PS) )
		{
			CPlayerState->ResetCharacterDefinition();
		}
	}

	bHeroSelectionStarted = false;

	if ( UCGameInstance* CGameInstance = GetGameInstance<UCGameInstance>() )
	{
		CGameInstance->SetSessionAdvertised(true);
		CGameInstance->UpdateAdvertisedHeroSelectionState(false);
	}

	OnPlayerSelectionUpdated.Broadcast(PlayerSelectionArray);
}

float ACGameState::GetMatchElapsedSeconds() const
{
	if ( MatchStartServerTime < 0.f )
		return 0.f;

	return GetServerWorldTimeSeconds() - MatchStartServerTime;
}

void ACGameState::RegisterFirstWaveSpawnTime()
{
	if ( !HasAuthority() || FirstWaveSpawnServerTime >= 0.f )
		return;

	FirstWaveSpawnServerTime = GetServerWorldTimeSeconds();
	ForceNetUpdate();
}

float ACGameState::GetTimeSinceFirstWave() const
{
	if ( FirstWaveSpawnServerTime < 0.f )
		return 0.f;

	return GetServerWorldTimeSeconds() - FirstWaveSpawnServerTime;
}

void ACGameState::SetFirstWavePlannedTime(float DelayFromNow)
{
	if ( !HasAuthority() )
		return;

	FirstWavePlannedServerTime = GetServerWorldTimeSeconds() + FMath::Max(0.f, DelayFromNow);
	ForceNetUpdate();
}

float ACGameState::GetTimeUntilFirstWave() const
{
	if ( FirstWavePlannedServerTime < 0.f )
		return -1.f;

	return FMath::Max(0.f, FirstWavePlannedServerTime - GetServerWorldTimeSeconds());
}

bool ACGameState::GetBaseLocationForTeam(uint8 TeamId, FVector& OutLocation, FRotator& OutRotation) const
{
	for (TActorIterator<ABaseZone> It(GetWorld()); It; ++It)
	{
		if (It->GetTeamId() == TeamId)
		{
			OutLocation = It->GetActorLocation(); // - It->GetTriggerVolumeHalfHeight();
			OutRotation = It->GetActorRotation();
			
			return true;
		}
	}

	return false;
}

void ACGameState::SetMatchPhase(EMatchPhase NewPhase, float CountdownDuration, float AnnounceDuration)
{
	if ( !HasAuthority() || MatchPhase == NewPhase )
		return;

	MatchPhase = NewPhase;

	if ( NewPhase == EMatchPhase::Countdown )
	{
		CountdownNumberDuration = CountdownDuration;
		CountdownEndServerTime = GetServerWorldTimeSeconds() + AnnounceDuration + CountdownDuration;
	}
	else if ( NewPhase == EMatchPhase::InProgress )
	{
		MatchStartServerTime = GetServerWorldTimeSeconds();
	}

	ForceNetUpdate();
	OnMatchPhaseChanged.Broadcast(MatchPhase);
}

float ACGameState::GetCountdownRemaining() const
{
	if ( MatchPhase != EMatchPhase::Countdown || CountdownEndServerTime < 0.f )
		return 0.f;

	return FMath::Max(0.f, CountdownEndServerTime - GetServerWorldTimeSeconds());
}

bool ACGameState::IsMatchInProgress(const UObject* WorldContextObject)
{
	UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	const ACGameState* CGameState = World ? World->GetGameState<ACGameState>() : nullptr;

	return !CGameState || CGameState->GetMatchPhase() == EMatchPhase::InProgress;
}

void ACGameState::OnRep_PlayerSelectionArray()
{
	OnPlayerSelectionUpdated.Broadcast(PlayerSelectionArray);
}

void ACGameState::OnRep_HeroSelectionReaminingTime()
{
	OnHeroSelectionTimerUpdated.Broadcast(HeroSelectionRemainingTime);
}

void ACGameState::StartHeroSelectionTimer()
{
	if ( !HasAuthority() )
		return;

	bHeroSelectionStarted = true;

	bHeroSelectionTimeOut = false;
	HeroSelectionRemainingTime = 90;

	GetWorld()->GetTimerManager().SetTimer(HeroSelectionTimerHandle, this, &ACGameState::UpdateHeroSelectionTimer, 1.f, true);
}

void ACGameState::RemovePlayerSelection(const APlayerState* LeavingPlayer)
{
	if ( !HasAuthority() )
		return;

	PlayerSelectionArray.RemoveAll(
		[ & ] (const FPlayerSelection& S)
		{
			return S.IsForPlayer(LeavingPlayer);
		}
	);
	
	OnPlayerSelectionUpdated.Broadcast(PlayerSelectionArray);
	ForceNetUpdate();
}

const FPlayerSelection* ACGameState::FindNextHost(const APlayerState* ExcludePlayer) const
{	
	for ( const FPlayerSelection& Selection : PlayerSelectionArray )
	{
		if ( Selection.IsForPlayer(ExcludePlayer) )
			continue;

		return &Selection;
	}

	return nullptr;
}

void ACGameState::SetHostSelection(const FUniqueNetIdRepl& NewHostId, const FString& NewHostNickname)
{
	if ( !HasAuthority() )
		return;

	for ( FPlayerSelection& Selection : PlayerSelectionArray )
	{
		Selection.SetbIsHost(Selection.GetPlayerUniqueId() == NewHostId);
	}

	HostNickname = NewHostNickname;
	OnRep_HostNickname();

	OnPlayerSelectionUpdated.Broadcast(PlayerSelectionArray);
	ForceNetUpdate();

	if ( !NewHostNickname.IsEmpty() )
	{
		if ( UCGameInstance* CGameInstance = GetGameInstance<UCGameInstance>() )
		{
			CGameInstance->UpdateAdvertisedHostNickname(NewHostNickname);
		}
	}
}

void ACGameState::Multicast_BroadcastKillFeed_Implementation(const FKillFeedEntry& Entry)
{
	OnKillFeedEntryReceived.Broadcast(Entry);
}

void ACGameState::UpdateHeroSelectionTimer()
{
	if ( !HasAuthority() )
		return;

	--HeroSelectionRemainingTime;

	if ( HeroSelectionRemainingTime <= 0 )
	{
		GetWorld()->GetTimerManager().ClearTimer(HeroSelectionTimerHandle);

		bHeroSelectionTimeOut = true;
		ForceNetUpdate();

		OnHeroSelectionTimeOut.Broadcast(CanForceStartMatch());
	}
}

void ACGameState::OnRep_bHeroSelectionTimeOut()
{
	if( bHeroSelectionTimeOut )
		OnHeroSelectionTimeOut.Broadcast(CanForceStartMatch());
}

void ACGameState::OnRep_HostNickname()
{
	OnHostNicknameChanged.Broadcast(HostNickname);
}

void ACGameState::OnRep_MatchPhase()
{
	OnMatchPhaseChanged.Broadcast(MatchPhase);
}
