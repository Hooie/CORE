// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/LobbyWidget.h"
#include "Character/PA_CharacterDefinition.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/TileView.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/WidgetSwitcher.h"
#include "Framework/CAssetManager.h"
#include "Framework/CGameInstance.h"
#include "Framework/CGameState.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"
#include "Network/CNetStatics.h"
#include "Player/CPlayerState.h"
#include "Player/LobbyPlayerController.h"
#include "Widgets/AbilityListView.h"
#include "Widgets/CharacterDisplay.h"
#include "Widgets/CharacterEntryWidget.h"
#include "Widgets/PlayerTeamLayoutWidget.h"
#include "Widgets/TeamSelectionWidget.h"

void ULobbyWidget::NativeConstruct()
{
	Super::NativeConstruct();

	ClearAndPopulateTeamSelectionSlots();

	ConfigureGameState();

	CPlayerState = GetOwningPlayerState<ACPlayerState>();

	LobbyPlayerController = GetOwningPlayer<ALobbyPlayerController>();
	if (LobbyPlayerController)
	{
		LobbyPlayerController->OnSwitchToHeroSelection.BindUObject(this, &ULobbyWidget::SwitchToHeroSelection);
		LobbyPlayerController->OnHostStatusChanged.AddUObject(this, &ULobbyWidget::RefreshPlayerSelectionDisplay);
	}	

	CGameInstance = GetGameInstance<UCGameInstance>();
	if ( CGameInstance )
	{
		SessionNameText->SetText(FText::FromString(CGameInstance->GetCurrentJoinedSessionName().ToString()));
	}
	RefreshHostNicknameText();

	StartHeroSelectionButton->SetVisibility(ESlateVisibility::Hidden);
	StartHeroSelectionButton->SetIsEnabled(false);
	StartHeroSelectionButton->OnClicked.AddDynamic(this, &ULobbyWidget::StartHeroSelectionButtonClicked);

	StartMatchButton->SetVisibility(ESlateVisibility::Hidden);
	StartMatchButton->SetIsEnabled(false);
	StartMatchButton->OnClicked.AddDynamic(this, &ULobbyWidget::StartMatchButtonClicked);

	TeamSelectionReadyButton->SetVisibility(ESlateVisibility::Hidden);
	TeamSelectionReadyButton->OnClicked.AddDynamic(this, &ULobbyWidget::TeamSelectionReadyButtonClicked);

	CharacterSelectionReadyButton->SetVisibility(ESlateVisibility::Hidden);
	CharacterSelectionReadyButton->SetIsEnabled(false);
	CharacterSelectionReadyButton->OnClicked.AddDynamic(this, &ULobbyWidget::CharacterSelectionReadyButtonClicked);

	LeaveSessionBtn->OnClicked.AddDynamic(this, &ULobbyWidget::LeaveSessionBtnClicked);

	UCAssetManager::Get().PreloadLobbyCharacterAssets(FStreamableDelegate::CreateUObject(this, &ULobbyWidget::CharacterDefinitionLoaded));

	if (CharacterSelectionTileView)
	{		
		CharacterSelectionTileView->OnItemSelectionChanged().AddUObject(this, &ULobbyWidget::CharacterSelected);
	}

	if ( CGameState )
	{
		UpdatePlayerSelectionDisplay(CGameState->GetPlayerSelection());
	}

	SpawnCharacterDisplay();
}

void ULobbyWidget::ClearAndPopulateTeamSelectionSlots()
{
	TeamSelectionSlotGridPanel->ClearChildren();

	for (int i = 0; i < UCNetStatics::GetPlayerCountPerTeam() * 2; ++i)
	{
		UTeamSelectionWidget* NewSelectionSlot = CreateWidget<UTeamSelectionWidget>(this, TeamSelectionWidgetClass);
		if (NewSelectionSlot)
		{
			NewSelectionSlot->SetSlotID(i);

			UUniformGridSlot* NewGridSlot = TeamSelectionSlotGridPanel->AddChildToUniformGrid(NewSelectionSlot);
			if (NewGridSlot)
			{
				int Row = i % UCNetStatics::GetPlayerCountPerTeam();
				int Column = i < UCNetStatics::GetPlayerCountPerTeam() ? 0 : 1;

				NewGridSlot->SetRow(Row);
				NewGridSlot->SetColumn(Column);
			}

			NewSelectionSlot->OnSlotClicked.AddUObject(this, &ULobbyWidget::SlotSelected);
			
			TeamSelectionSlots.Add(NewSelectionSlot);
		}
	}
}

void ULobbyWidget::SlotSelected(uint8 NewSlotID)
{
	if (LobbyPlayerController)
	{
		LobbyPlayerController->Server_RequestSlotSelectionChange(NewSlotID);
	}
}

void ULobbyWidget::ConfigureGameState()
{
	UWorld* World = GetWorld();
	if (!World)
		return;

	CGameState = World->GetGameState<ACGameState>();
	if (!CGameState)
	{
		World->GetTimerManager().SetTimer(ConfigureGameStateTimerHandle, this, &ULobbyWidget::ConfigureGameState, 1.f);
	}
	else
	{
		CGameState->OnPlayerSelectionUpdated.AddUObject(this, &ULobbyWidget::UpdatePlayerSelectionDisplay);
		CGameState->OnHeroSelectionTimeOut.AddUObject(this, &ULobbyWidget::StopHeroSelection);
		CGameState->OnHeroSelectionTimerUpdated.AddUObject(this, &ULobbyWidget::HeroSelectionTimerUpdated);
		CGameState->OnHostNicknameChanged.AddUObject(this, &ULobbyWidget::RefreshHostNicknameText);
	}
}

void ULobbyWidget::UpdatePlayerSelectionDisplay(const TArray<FPlayerSelection>& PlayerSelections)
{
	for (UTeamSelectionWidget* SelectionSlot : TeamSelectionSlots)
	{
		SelectionSlot->UpdateSlotInfo(TEXT("비어있음"));
	}

	RefreshHostNicknameText();

	for (UUserWidget* CharacterEntryAsWidget : CharacterSelectionTileView->GetDisplayedEntryWidgets())
	{
		if (UCharacterEntryWidget* CharacterEntryWidget = Cast<UCharacterEntryWidget>(CharacterEntryAsWidget))
		{
			CharacterEntryWidget->SetSelected(false);
		}
	}

	int32 TeamSize = UCNetStatics::GetPlayerCountPerTeam();
	uint8 MySlot = 255;
	bool bLocalPlayerReady = false;

	for ( const FPlayerSelection& PlayerSelection : PlayerSelections )
		if ( PlayerSelection.IsForPlayer(GetOwningPlayerState()) )
		{
			MySlot = PlayerSelection.GetPlayerSlot();
			bLocalPlayerReady = PlayerSelection.GetbIsReady();
			break;
		}

	bool bAmITeamOne = MySlot < TeamSize;

	for (const FPlayerSelection& PlayerSelection : PlayerSelections)
	{
		if (!PlayerSelection.IsValid())
			continue;

		TeamSelectionSlots[ PlayerSelection.GetPlayerSlot() ]->UpdateSlotInfo(PlayerSelection.GetPlayerNickName(), PlayerSelection.GetbIsHost(), PlayerSelection.GetbIsReady());

		if ( !PlayerSelection.GetCharacterDefinition() )
			continue;

		bool bOtherTeamIsOne = PlayerSelection.GetPlayerSlot() < TeamSize;
		bool bIsSameTeam = (bAmITeamOne == bOtherTeamIsOne);

		UCharacterEntryWidget* SelectedEntry = CharacterSelectionTileView->GetEntryWidgetFromItem<UCharacterEntryWidget>(PlayerSelection.GetCharacterDefinition());
		if (SelectedEntry)
		{
			bool bShouldSelect = (MySlot != 255) && bIsSameTeam;

			if( bShouldSelect )
				SelectedEntry->SetSelected(true);
		}

		if (PlayerSelection.IsForPlayer(GetOwningPlayerState()))
		{
			if(PlayerSelection.GetCharacterDefinition() != CachedCharacterDefinition)
			{
				CachedCharacterDefinition = PlayerSelection.GetCharacterDefinition();
				UpdateCharacterDisplay(PlayerSelection);
			}
		}
	}

	bool bIsHost = LobbyPlayerController && LobbyPlayerController->GetbIsHost();

	for ( UTeamSelectionWidget* SlotWidget : TeamSelectionSlots )
	{
		SlotWidget->SetSelectButtonEnabled(!bLocalPlayerReady || bIsHost);
	}

	if (CGameState)
	{
		if ( bIsHost )
		{
			bool bIsTeamSelectionActive = MainSwitcher->GetActiveWidget() == TeamSelectionRoot;

			if ( bIsTeamSelectionActive )
			{
				StartHeroSelectionButton->SetVisibility(ESlateVisibility::Visible);
				StartHeroSelectionButton->SetIsEnabled(CGameState->CanStartHeroSelection());
			}

			StartMatchButton->SetVisibility(ESlateVisibility::Visible);
			StartMatchButton->SetIsEnabled(CGameState->CanStartMatch());

			TeamSelectionReadyButton->SetVisibility(ESlateVisibility::Hidden);
			CharacterSelectionReadyButton->SetVisibility(ESlateVisibility::Hidden);
		}
		else
		{
			TeamSelectionReadyButton->SetVisibility(ESlateVisibility::Visible);
			CharacterSelectionReadyButton->SetVisibility(ESlateVisibility::Visible);

			StartHeroSelectionButton->SetVisibility(ESlateVisibility::Hidden);
			StartMatchButton->SetVisibility(ESlateVisibility::Hidden);
		}
	}

	bool bIsHeroSelectionActive = MainSwitcher->GetActiveWidget() == HeroSelectionRoot;

	if ( !bIsHost )
	{
		bool bEnabled = !bIsHeroSelectionActive || !bLocalPlayerReady;

		CharacterSelectionTileView->SetIsEnabled(bEnabled);
	}

	if (PlayerTeamLayoutWidget)
	{
		PlayerTeamLayoutWidget->UpdatePlayerSelection(PlayerSelections);
	}
}

void ULobbyWidget::RefreshPlayerSelectionDisplay()
{
	if ( CGameState )
	{
		UpdatePlayerSelectionDisplay(CGameState->GetPlayerSelection());
	}
}

void ULobbyWidget::StartHeroSelectionButtonClicked()
{
	if (LobbyPlayerController)
	{
		LobbyPlayerController->Server_StartHeroSelection();
	}
}

void ULobbyWidget::TeamSelectionReadyButtonClicked()
{
	bIsLocalPlayerReady = !bIsLocalPlayerReady;

	LobbyPlayerController->Server_SetReady(bIsLocalPlayerReady);

	UpdateReadyButtonText(TeamSelectionReadyText);
}

void ULobbyWidget::CharacterSelectionReadyButtonClicked()
{
	bIsLocalPlayerReady = !bIsLocalPlayerReady;

	LobbyPlayerController->Server_SetReady(bIsLocalPlayerReady);

	UpdateReadyButtonText(CharacterSelectionReadyText);
}

void ULobbyWidget::SwitchToTeamSelection()
{
	CharacterDisplay->Destroy();
	CharacterDisplay = nullptr;
	SpawnCharacterDisplay();

	bIsLocalPlayerReady = false;
	UpdateReadyButtonText(TeamSelectionReadyText);
	
	CachedCharacterDefinition = nullptr;

	if ( !LobbyPlayerController )
		return;
	
	LobbyPlayerController->Server_SetReady(false);

	if( LobbyPlayerController->GetbIsHost() )
	{
		StartHeroSelectionButton->SetVisibility(ESlateVisibility::Visible);
		StartHeroSelectionButton->SetIsEnabled(CGameState->CanStartHeroSelection());

		LobbyPlayerController->Server_ResetCharacterSelections();
	}

	MainSwitcher->SetActiveWidget(TeamSelectionRoot);
}

void ULobbyWidget::SwitchToHeroSelection()
{
	bIsLocalPlayerReady = false;
	UpdateReadyButtonText(CharacterSelectionReadyText);

	CharacterSelectionReadyButton->SetIsEnabled(false);
	
	CharacterSelectionTileView->SetIsEnabled(true);

	CharacterSelectionTileView->ClearSelection();

	RefreshPlayerSelectionDisplay();

	MainSwitcher->SetActiveWidget(HeroSelectionRoot);
}

void ULobbyWidget::CharacterDefinitionLoaded()
{
	TArray<UPA_CharacterDefinition*> LoadedCharacterDefinitions;

	if (UCAssetManager::Get().GetLoadedCharacterDefinitions(LoadedCharacterDefinitions))
	{
		CharacterSelectionTileView->SetListItems(LoadedCharacterDefinitions);
	}
}

void ULobbyWidget::RefreshHostNicknameText(const FString& NewHostNickname)
{
	if ( !HostNicknameText )
		return;

	if ( LobbyPlayerController && LobbyPlayerController->GetbIsHost() && CGameInstance )
	{
		HostNicknameText->SetText(FText::FromString(CGameInstance->GetCachedPlayerNickname()));
		return;
	}

	if ( CGameState )
	{
		HostNicknameText->SetText(FText::FromString(CGameState->GetHostNickname()));
	}
}

void ULobbyWidget::CharacterSelected(UObject* SelectedUObject)
{
	if ( !SelectedUObject )
		return;

	if ( bIsLocalPlayerReady )
		return;

	if (!CPlayerState)
		return;

	if (const UPA_CharacterDefinition* CharacterDefinition = Cast<UPA_CharacterDefinition>(SelectedUObject))
	{
		CPlayerState->Server_SetSelectedCharacterDefinition(CharacterDefinition);

		CharacterSelectionReadyButton->SetIsEnabled(true);
	}
}

void ULobbyWidget::SpawnCharacterDisplay()
{
	if (CharacterDisplay)
		return;

	if (!CharacterDisplayClass)
		return;

	FTransform CharacterDisplayTransform = FTransform::Identity;

	AActor* PlayerStart = UGameplayStatics::GetActorOfClass(GetWorld(), APlayerStart::StaticClass());
	if (PlayerStart)
	{
		CharacterDisplayTransform = PlayerStart->GetActorTransform();
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	CharacterDisplay = GetWorld()->SpawnActor<ACharacterDisplay>(CharacterDisplayClass, CharacterDisplayTransform, SpawnParams);
	
	GetOwningPlayer()->SetViewTarget(CharacterDisplay);
}

void ULobbyWidget::UpdateCharacterDisplay(const FPlayerSelection& PlayerSelection)
{
	if (!PlayerSelection.GetCharacterDefinition())
		return;

	CharacterDisplay->ConfigureWithCharacterDefinition(PlayerSelection.GetCharacterDefinition());
	AbilityListView->ClearListItems();
	const TMap<ECAbilityInputID, TSubclassOf<UGameplayAbility>>* Abilities = PlayerSelection.GetCharacterDefinition()->GetAbilities();
	if (Abilities)
	{
		AbilityListView->ConfigureAbilities(*Abilities);
	}
}

void ULobbyWidget::StartMatchButtonClicked()
{
	if (LobbyPlayerController)
	{
		LobbyPlayerController->Server_RequestStartMatch();
	}
}

void ULobbyWidget::UpdateReadyButtonText(UTextBlock* TextBlock)
{
	TextBlock->SetText(bIsLocalPlayerReady ? FText::FromString(TEXT("준비 해제")) : FText::FromString(TEXT("준비")));
}

void ULobbyWidget::StopHeroSelection(bool CanForceStartMatch)
{
	if ( !CanForceStartMatch )
	{
		SwitchToTeamSelection();
	}
	else
	{
		if ( LobbyPlayerController && LobbyPlayerController->GetbIsHost() )
		{
			LobbyPlayerController->Server_RequestStartMatch();
		}
	}
}

void ULobbyWidget::HeroSelectionTimerUpdated(int32 RemainingTime)
{
	HeroSelectionTimerText->SetText(FText::FromString(FString::FromInt(RemainingTime)));

	if ( RemainingTime < 10 )
		HeroSelectionTimerText->SetColorAndOpacity(FSlateColor(FLinearColor::Red));
	else
	{
		HeroSelectionTimerText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
	}
}

void ULobbyWidget::LeaveSessionBtnClicked()
{
	if ( LobbyPlayerController )
	{
		LobbyPlayerController->Server_RequestLeaveSession();
	}
}
