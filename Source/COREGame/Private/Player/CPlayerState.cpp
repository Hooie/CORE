// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/CPlayerState.h"
#include "Character/CCharacter.h"
#include "Character/PA_CharacterDefinition.h"
#include "Framework/CGameState.h"
#include "Net/UnrealNetwork.h"
#include "Network/CNetStatics.h"
#include "Kismet/GameplayStatics.h"

ACPlayerState::ACPlayerState()
{
	bReplicates = true;
	//NetUpdateFrequency = 100.f;
	SetNetUpdateFrequency(10.f);
}

void ACPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ACPlayerState, PlayerSelection);
	DOREPLIFETIME(ACPlayerState, Kills);
	DOREPLIFETIME(ACPlayerState, Deaths);
	DOREPLIFETIME(ACPlayerState, Assists);
	DOREPLIFETIME(ACPlayerState, MinionKills);
	DOREPLIFETIME(ACPlayerState, ScoreboardItems);
	DOREPLIFETIME(ACPlayerState, RespawnTimeStamp);

	DOREPLIFETIME(ACPlayerState, LoadingProgress);
	DOREPLIFETIME(ACPlayerState, bLoadingComplete);
}

void ACPlayerState::BeginPlay()
{
	Super::BeginPlay();

	CGameState = Cast<ACGameState>(UGameplayStatics::GetGameState(this));

	if (CGameState)
	{
		CGameState->OnPlayerSelectionUpdated.AddUObject(this, &ACPlayerState::PlayerSelectionUpdated);
	}
}

void ACPlayerState::CopyProperties(APlayerState* PlayerState)
{
	Super::CopyProperties(PlayerState);

	ACPlayerState* NewPlayerState = Cast<ACPlayerState>(PlayerState);
	if (NewPlayerState)
	{
		NewPlayerState->PlayerSelection = PlayerSelection;
	}
}

TSubclassOf<APawn> ACPlayerState::GetSelectedPawnClass() const
{
	if (PlayerSelection.GetCharacterDefinition())
	{
		return PlayerSelection.GetCharacterDefinition()->LoadCharacterClass();
	}

	return nullptr;
}

FGenericTeamId ACPlayerState::GetTeamIdBasedOnSlot() const
{
	return PlayerSelection.GetPlayerSlot() < UCNetStatics::GetPlayerCountPerTeam() ? FGenericTeamId{ 0 } : FGenericTeamId{ 1 };
}

void ACPlayerState::ResetCharacterDefinition()
{
	PlayerSelection.SetCharacterDefinition(nullptr);
}

void ACPlayerState::AddKill()
{
	if ( HasAuthority() )
	{
		++Kills;

		ForceNetUpdate();
	}
}

void ACPlayerState::AddDeath()
{
	if ( HasAuthority() )
	{
		++Deaths;
		
		ForceNetUpdate();
	}
}

void ACPlayerState::AddAssist()
{
	if ( HasAuthority() )
	{
		++Assists;

		ForceNetUpdate();
	}
}

void ACPlayerState::AddMinionKill()
{
	if ( HasAuthority() )
	{
		++MinionKills;

		ForceNetUpdate();
	}
}

void ACPlayerState::UpdateScoreboardItems(const TArray<FScoreboardItemEntry>& NewItems)
{
	if ( HasAuthority() )
	{
		ScoreboardItems = NewItems;

		ForceNetUpdate();
	}
}

void ACPlayerState::SetRespawnTimeStamp(float NewTimeStamp)
{
	RespawnTimeStamp = NewTimeStamp;

	ForceNetUpdate();
}

void ACPlayerState::SetLoadingProgress(uint8 NewProgress)
{
	if ( !HasAuthority() || bLoadingComplete || LoadingProgress == NewProgress )
		return;

	LoadingProgress = NewProgress;
	ForceNetUpdate();
}

void ACPlayerState::SetLoadingComplete()
{
	if ( !HasAuthority() || bLoadingComplete )
		return;

	bLoadingComplete = true;
	LoadingProgress = 100;
	ForceNetUpdate();
}

void ACPlayerState::OnRep_PlayerName()
{
	Super::OnRep_PlayerName();

	if ( ACCharacter* Character = GetPawn<ACCharacter>() )
	{
		Character->UpdateOverheadName();
	}
}

void ACPlayerState::Server_SetSelectedCharacterDefinition_Implementation(const UPA_CharacterDefinition* NewDefinition)
{
	if (!CGameState || !NewDefinition)
		return;

	if (CGameState->IsDefinitionSelectedByTeam(NewDefinition, PlayerSelection.GetPlayerSlot()) )
	{
		UE_LOG(LogTemp, Warning, TEXT("Rejected - Already selected by same team"));

		return;
	}

	if (PlayerSelection.GetCharacterDefinition())
	{
		UE_LOG(LogTemp, Warning, TEXT("Deselecting previous: %s"),
			*PlayerSelection.GetCharacterDefinition()->GetName());

		CGameState->SetCharacterDeselectedForPlayer(this);
	}

	PlayerSelection.SetCharacterDefinition(NewDefinition);
	CGameState->SetCharacterSelection(this, NewDefinition);
}

bool ACPlayerState::Server_SetSelectedCharacterDefinition_Validate(const UPA_CharacterDefinition* NewDefinition)
{
	return true;
}


void ACPlayerState::PlayerSelectionUpdated(const TArray<FPlayerSelection>& NewPlayerSelections)
{
	for (const FPlayerSelection& NewPlayerSelection : NewPlayerSelections)
	{
		if (NewPlayerSelection.IsForPlayer(this))
		{
			PlayerSelection = NewPlayerSelection;
		}
	}
}
