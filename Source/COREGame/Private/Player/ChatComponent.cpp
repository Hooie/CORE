// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/ChatComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GenericTeamAgentInterface.h"
#include "Framework/CGameState.h"
#include "Network/CNetStatics.h"

UChatComponent::UChatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	SetIsReplicatedByDefault(true);
}

void UChatComponent::Server_SendChatMessage_Implementation(const FString& MessageText, EChatChannel Channel)
{
	APlayerController* SenderPC = Cast<APlayerController>(GetOwner());
	if ( !SenderPC || !SenderPC->PlayerState )
		return;

	FString Trimmed = MessageText.Left(MaxMessageLength).TrimStartAndEnd();
	if ( Trimmed.IsEmpty() )
		return;

	FDateTime Now = FDateTime::Now();

	FChatMessage NewMessage;
	NewMessage.SenderNickname = SenderPC->PlayerState->GetPlayerName();
	NewMessage.MessageText = Trimmed;
	NewMessage.Channel = Channel;
	NewMessage.SenderTeamId = ResolveTeamId(SenderPC->PlayerState);
	NewMessage.Hour = Now.GetHour();
	NewMessage.Minute = Now.GetMinute();
	NewMessage.Second = Now.GetSecond();

	UWorld* World = GetWorld();
	if ( !World )
		return;

	for ( FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It )
	{
		APlayerController* TargetPC = It->Get();
		if ( !TargetPC )
			continue;

		UChatComponent* TargetChatComponent = TargetPC->FindComponentByClass<UChatComponent>();
		if ( !TargetChatComponent )
			continue;

		if ( Channel == EChatChannel::All || TargetChatComponent->ResolveTeamId(TargetPC->PlayerState) == NewMessage.SenderTeamId )
		{
			TargetChatComponent->Client_ReceiveChatMessage(NewMessage);
		}
	}
}

bool UChatComponent::Server_SendChatMessage_Validate(const FString& MessageText, EChatChannel Channel)
{
	return true;
}

uint8 UChatComponent::GetOwnerTeamId() const
{
	APlayerController* PC = Cast<APlayerController>(GetOwner());

	return ResolveTeamId(PC ? PC->PlayerState : nullptr);
}

void UChatComponent::Client_ReceiveChatMessage_Implementation(const FChatMessage& Message)
{
	OnChatMessageReceived.Broadcast(Message);
}

uint8 UChatComponent::ResolveTeamId(APlayerState* PlayerState) const
{
	IGenericTeamAgentInterface* TeamInterface = Cast<IGenericTeamAgentInterface>(GetOwner());
	if ( TeamInterface )
	{
		return TeamInterface->GetGenericTeamId().GetId();
	}

	if ( UWorld* World = GetWorld() )
	{
		if ( ACGameState* CGameState = World->GetGameState<ACGameState>() )
		{
			int32 TeamSize = UCNetStatics::GetPlayerCountPerTeam();

			for ( const FPlayerSelection& Selection : CGameState->GetPlayerSelection() )
			{
				if ( Selection.IsForPlayer(PlayerState) )
				{
					return Selection.GetPlayerSlot() < TeamSize ? 0 : 1;
				}
			}
		}
	}

	return 0;
}