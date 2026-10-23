// Fill out your copyright notice in the Description page of Project Settings.


#include "Framework/LobbyGameMode.h"
#include "Framework/CGameState.h"
#include "Network/CGameSession.h"

ALobbyGameMode::ALobbyGameMode()
{
	bUseSeamlessTravel = true;
	GameSessionClass = ACGameSession::StaticClass();
}

void ALobbyGameMode::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	Super::PreLogin(Options, Address, UniqueId, ErrorMessage);

	if ( !ErrorMessage.IsEmpty() )
		return;

	if ( ACGameState* CGameState = GetGameState<ACGameState>() )
	{
		if ( CGameState->IsHeroSelectionStarted() )
		{
			ErrorMessage = TEXT("HeroSelectionAlreadyStarted");
		}
	}
}
