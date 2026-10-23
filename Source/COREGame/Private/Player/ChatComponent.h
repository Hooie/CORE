// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Player/ChatTypes.h"
#include "ChatComponent.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnChatMessageReceived, const FChatMessage& /*ChatMessage*/);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UChatComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UChatComponent();

	FOnChatMessageReceived OnChatMessageReceived;
		
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SendChatMessage(const FString& MessageText, EChatChannel Channel);

	uint8 GetOwnerTeamId() const;

private:
	UFUNCTION(Client, Reliable)
	void Client_ReceiveChatMessage(const FChatMessage& Message);

	uint8 ResolveTeamId(APlayerState* PlayerState) const;

	UPROPERTY(EditDefaultsOnly, Category = "Chat")
	int32 MaxMessageLength = 200;
};
