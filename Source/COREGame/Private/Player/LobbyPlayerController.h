// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Player/MenuPlayerController.h"
#include "LobbyPlayerController.generated.h"

DECLARE_DELEGATE(FOnSwitchToHeroSelection);
DECLARE_MULTICAST_DELEGATE(FOnHostStatusChanged);

/**
 * 
 */
UCLASS()
class ALobbyPlayerController : public AMenuPlayerController
{
	GENERATED_BODY()
	
public:
	ALobbyPlayerController();

	virtual void GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const override;

	FOnSwitchToHeroSelection OnSwitchToHeroSelection;
	FOnHostStatusChanged OnHostStatusChanged;

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_RequestSlotSelectionChange(uint8 NewSlotID);

	UFUNCTION(Server, Reliable)
	void Server_ResetCharacterSelections();

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_StartHeroSelection();

	UFUNCTION(Client, Reliable)
	void Client_StartHeroSelection();
	
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_RequestStartMatch();

	UFUNCTION(Server, Reliable)
	void Server_RequestLeaveSession();

	UFUNCTION(Client, Reliable)
	void Client_ConfirmLeaveSession();

	void SetbIsHost(bool InbIsHost) { bIsHost = InbIsHost; }
	bool GetbIsHost() { return bIsHost; }

	UFUNCTION(Server, Reliable)
	void Server_SetReady(bool bReady);

	bool GetIsReady() const;

private:
	UPROPERTY(VisibleAnywhere, Category = "Chat")
	class UChatComponent* ChatComponent;

	UPROPERTY(ReplicatedUsing = OnRep_bIsHost)
	bool bIsHost;

	UFUNCTION()
	void OnRep_bIsHost();
};
