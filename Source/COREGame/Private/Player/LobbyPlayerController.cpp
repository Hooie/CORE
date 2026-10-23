// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/LobbyPlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Framework/CGameInstance.h"
#include "Framework/CGameState.h"
#include "Net/UnrealNetwork.h"

ALobbyPlayerController::ALobbyPlayerController()
{
	bAutoManageActiveCameraTarget = false;
	
	ChatComponent = CreateDefaultSubobject<UChatComponent>("Chat Component");
}

void ALobbyPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ALobbyPlayerController, bIsHost);
}

void ALobbyPlayerController::Server_ResetCharacterSelections_Implementation()
{
	ACGameState* CGameState = GetWorld()->GetGameState<ACGameState>();
	if ( CGameState )
	{
		CGameState->ResetAllCharacterSelections();
	}
}

void ALobbyPlayerController::Server_SetReady_Implementation(bool bReady)
{
	ACGameState* CGameState = GetWorld()->GetGameState<ACGameState>();
	if ( CGameState && PlayerState )
		CGameState->SetPlayerReady(PlayerState, bReady);
}

bool ALobbyPlayerController::GetIsReady() const
{
	ACGameState* CGameState = GetWorld()->GetGameState<ACGameState>();
	if ( CGameState && PlayerState )
		return CGameState->GetIsPlayerReady(PlayerState);
	
	return false;
}

void ALobbyPlayerController::OnRep_bIsHost()
{
	OnHostStatusChanged.Broadcast();
}

void ALobbyPlayerController::Server_RequestSlotSelectionChange_Implementation(uint8 NewSlotID)
{
	if (!GetWorld())
		return;

	ACGameState* CGameState = GetWorld()->GetGameState<ACGameState>();
	if (!CGameState)
		return;

	CGameState->RequestPlayerSelectionChange(GetPlayerState<APlayerState>(), NewSlotID);
}

bool ALobbyPlayerController::Server_RequestSlotSelectionChange_Validate(uint8 NewSlotID)
{
	return true;
}

void ALobbyPlayerController::Server_StartHeroSelection_Implementation()
{
	if (!HasAuthority() || !GetWorld())
		return;

	ACGameState* CGameState = GetWorld()->GetGameState<ACGameState>();
	if ( CGameState )
	{
		CGameState->ResetAllPlayersReady();
		CGameState->StartHeroSelectionTimer();
	}

	if ( UCGameInstance* CGameInstance = GetGameInstance<UCGameInstance>() )
	{
		CGameInstance->SetSessionAdvertised(false);
		CGameInstance->UpdateAdvertisedHeroSelectionState(true);
	}

	for (FConstPlayerControllerIterator PlayerControllerIterator = GetWorld()->GetPlayerControllerIterator(); PlayerControllerIterator; ++PlayerControllerIterator)
	{
		ALobbyPlayerController* PlayerController = Cast<ALobbyPlayerController>(*PlayerControllerIterator);
		if (PlayerController)
		{
			PlayerController->Client_StartHeroSelection();
		}
	}
}

bool ALobbyPlayerController::Server_StartHeroSelection_Validate()
{
	return true;
}

void ALobbyPlayerController::Client_StartHeroSelection_Implementation()
{
	OnSwitchToHeroSelection.ExecuteIfBound();
}

void ALobbyPlayerController::Server_RequestStartMatch_Implementation()
{
	UCGameInstance* CGameInstance = GetGameInstance<UCGameInstance>();
	if ( !CGameInstance )
		return;

	FUniqueNetIdRepl RequesterId;
	if ( PlayerState )
	{
		RequesterId = PlayerState->GetUniqueId();
	}

	if ( RequesterId != CGameInstance->GetHostPlayerId() )
	{
		UE_LOG(LogTemp, Warning, TEXT("Non-Host tried to start match, Rejected!"));
		return;
	}

	CGameInstance->StartMatch();
}

bool ALobbyPlayerController::Server_RequestStartMatch_Validate()
{
	return true;
}

void ALobbyPlayerController::Server_RequestLeaveSession_Implementation()
{
	UWorld* World = GetWorld();
	if ( !World )
		return;

	ACGameState* CGameState = World->GetGameState<ACGameState>();
	UCGameInstance* CGameInstance = GetGameInstance<UCGameInstance>();

	if ( CGameState && CGameInstance )
	{
		if(GetbIsHost() )
		{
			if ( const FPlayerSelection* NextHostSelection = CGameState->FindNextHost(PlayerState) )
			{
				FUniqueNetIdRepl NextHostId = NextHostSelection->GetPlayerUniqueId();
				FUniqueNetIdRepl PreviousHostId = PlayerState ? PlayerState->GetUniqueId() : FUniqueNetIdRepl();

				CGameInstance->TransferHost(NextHostId);
				CGameState->SetHostSelection(NextHostId, NextHostSelection->GetPlayerNickName());
				CGameInstance->UpdateHostPlayerController(NextHostId, PreviousHostId);
			}
		}
		CGameState->RemovePlayerSelection(PlayerState);
	}
	Client_ConfirmLeaveSession();
}

void ALobbyPlayerController::Client_ConfirmLeaveSession_Implementation()
{
	if ( UCGameInstance* CGameInstance = GetGameInstance<UCGameInstance>() )
	{
		CGameInstance->LeaveCurrentSessionAndReturnToMainMenu();
	}
}
