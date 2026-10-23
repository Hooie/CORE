// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Player/PlayerInfoTypes.h"
#include "LobbyWidget.generated.h"

/**
 * 
 */
UCLASS()
class ULobbyWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	virtual void NativeConstruct() override;

private:
	UPROPERTY(meta=(BindWidget))
	class UWidgetSwitcher* MainSwitcher;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* HostNicknameText;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* SessionNameText;

	UPROPERTY(meta = (BindWidget))
	class UChatWidget* ChatWidget;

	UPROPERTY()
	class ALobbyPlayerController* LobbyPlayerController;

	UPROPERTY()
	class ACPlayerState* CPlayerState;

	UPROPERTY()
	class ACGameState* CGameState;

	UPROPERTY()
	class UCGameInstance* CGameInstance;

	void ConfigureGameState();

	FTimerHandle ConfigureGameStateTimerHandle;

	void UpdatePlayerSelectionDisplay(const TArray<FPlayerSelection>& PlayerSelection);

	void RefreshPlayerSelectionDisplay();

	void CharacterDefinitionLoaded();

	bool bIsLocalPlayerReady = false;

	void RefreshHostNicknameText(const FString& NewHostNickname = FString());

/********************************************************/
/*********************Team Selection*********************/
/********************************************************/

	UPROPERTY(meta=(BindWidget))
	class UWidget* TeamSelectionRoot;

	UPROPERTY(meta=(BindWidget))
	class UButton* StartHeroSelectionButton;

	UPROPERTY(meta=(BindWidget))
	class UUniformGridPanel* TeamSelectionSlotGridPanel;

	UPROPERTY(EditDefaultsOnly, Category = "TeamSelection")
	TSubclassOf<class UTeamSelectionWidget> TeamSelectionWidgetClass;

	UPROPERTY()
	TArray<class UTeamSelectionWidget*> TeamSelectionSlots;

	void ClearAndPopulateTeamSelectionSlots();
	void SlotSelected(uint8 NewSlotID);

	UPROPERTY(meta = (BindWidget))
	class UButton* TeamSelectionReadyButton;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* TeamSelectionReadyText;

	UFUNCTION()
	void TeamSelectionReadyButtonClicked();

	void SwitchToTeamSelection();

	UFUNCTION()
	void StartHeroSelectionButtonClicked();

	void UpdateReadyButtonText(UTextBlock* TextBlock);

	UPROPERTY(meta = (BindWidget))
	UButton* LeaveSessionBtn;

	UFUNCTION()
	void LeaveSessionBtnClicked();
	
/********************************************************/
/*********************Hero Selection*********************/
/********************************************************/

	UPROPERTY(meta = (BindWidget))
	class UWidget* HeroSelectionRoot;

	UPROPERTY(meta = (BindWidget))
	class UTileView* CharacterSelectionTileView;

	UPROPERTY(meta = (BindWidget))
	class UAbilityListView* AbilityListView;

	UPROPERTY(meta = (BindWidget))
	class UPlayerTeamLayoutWidget* PlayerTeamLayoutWidget;

	UPROPERTY(meta = (BindWidget))
	class UButton* CharacterSelectionReadyButton;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* CharacterSelectionReadyText;

	UPROPERTY(meta = (BindWidget))
	class UButton* StartMatchButton;
	
	UFUNCTION()
	void CharacterSelectionReadyButtonClicked();

	void SwitchToHeroSelection();

	void CharacterSelected(UObject* SelectedUObject);

	UPROPERTY(EditDefaultsOnly, Category = "Character Display")
	TSubclassOf<class ACharacterDisplay> CharacterDisplayClass;

	UPROPERTY()
	class ACharacterDisplay* CharacterDisplay;

	void SpawnCharacterDisplay();
	void UpdateCharacterDisplay(const FPlayerSelection& PlayerSelection);

	UFUNCTION()
	void StartMatchButtonClicked();

	UPROPERTY()
	const UPA_CharacterDefinition* CachedCharacterDefinition;

	UPROPERTY(meta=(BindWidget))
	UTextBlock* HeroSelectionTimerText;

	void StopHeroSelection(bool CanForceStartMatch);
	void HeroSelectionTimerUpdated(int32 RemainingTime);
};
