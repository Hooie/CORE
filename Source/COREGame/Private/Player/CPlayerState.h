// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "GenericTeamAgentInterface.h"
#include "Inventory/PA_ShopItem.h"
#include "Player/PlayerInfoTypes.h"
#include "CPlayerState.generated.h"

class UPA_CharacterDefinition;

USTRUCT(BlueprintType)
struct FScoreboardItemEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	const UPA_ShopItem* ShopItem = nullptr;

	UPROPERTY(BlueprintReadOnly)
	int32 StackCount = 1;
};

/**
 * 
 */
UCLASS()
class ACPlayerState : public APlayerState
{
	GENERATED_BODY()
	
public:
	ACPlayerState();
	virtual void GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void CopyProperties(APlayerState* PlayerState) override;

	TSubclassOf<APawn> GetSelectedPawnClass() const;
	FGenericTeamId GetTeamIdBasedOnSlot() const;

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SetSelectedCharacterDefinition(const UPA_CharacterDefinition* NewDefinition);

	void ResetCharacterDefinition();

	const FPlayerSelection& GetPlayerSelection() const { return PlayerSelection; }

	void AddKill();
	void AddDeath();
	void AddAssist();
	void AddMinionKill();

	int32 GetKills() const { return Kills; }
	int32 GetDeath() const { return Deaths; }
	int32 GetAssists() const {return Assists; }
	int32 GetMinionKills() const { return MinionKills; }

	void UpdateScoreboardItems(const TArray<FScoreboardItemEntry>& NewItems);
	const TArray<FScoreboardItemEntry>& GetScoreboardItems() const { return ScoreboardItems; }

	void SetRespawnTimeStamp(float NewTimeStamp);
	float GetRespawnTimeStamp() const { return RespawnTimeStamp; }

	void SetLoadingProgress(uint8 NewProgress);
	void SetLoadingComplete();
	uint8 GetLoadingProgress() const { return LoadingProgress; }
	bool IsLoadingComplete() const { return bLoadingComplete; }

	virtual void OnRep_PlayerName() override;

private:
	UPROPERTY(Replicated)
	FPlayerSelection PlayerSelection;

	UPROPERTY()
	class ACGameState* CGameState;

	void PlayerSelectionUpdated(const TArray<FPlayerSelection>& NewPlayerSelections);

	UPROPERTY(Replicated)
	int32 Kills = 0;

	UPROPERTY(Replicated)
	int32 Deaths = 0;

	UPROPERTY(Replicated)
	int32 Assists = 0;

	UPROPERTY(Replicated)
	int32 MinionKills = 0;

	UPROPERTY(Replicated)
	TArray<FScoreboardItemEntry> ScoreboardItems;

	UPROPERTY(Replicated)
	float RespawnTimeStamp = -1.f;

	UPROPERTY(Replicated)
	uint8 LoadingProgress = 0;

	UPROPERTY(Replicated)
	bool bLoadingComplete = false;
};
