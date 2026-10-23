// Fill out your copyright notice in the Description page of Project Settings.

#include "Widgets/ScoreboardWidget.h"
#include "Components/VerticalBox.h"
#include "Components/TextBlock.h"
#include "Framework/CGameState.h"
#include "GenericTeamAgentInterface.h"
#include "Player/CPlayerState.h"
#include "Widgets/ScoreboardEntryWidget.h"

void UScoreboardWidget::RefreshScoreboard()
{
	BuildEntriesIfNeeded(); 

	int32 TeamOneKills = 0;
	int32 TeamTwoKills = 0;

	for ( const TPair<ACPlayerState*, UScoreboardEntryWidget*>& Pair : EntryWidgets )
	{
		ACPlayerState* PS = Pair.Key;
		if ( !PS )
			continue;

		Pair.Value->RefreshStats(); 

		uint8 TeamId = PS->GetTeamIdBasedOnSlot().GetId();

		if ( TeamId == 0 )
			TeamOneKills += PS->GetKills();
		else
			TeamTwoKills += PS->GetKills();
	}

	if ( TeamOneTotalKillCountText )
		TeamOneTotalKillCountText->SetText(FText::AsNumber(TeamOneKills));

	if ( TeamTwoTotalKillCountText )
		TeamTwoTotalKillCountText->SetText(FText::AsNumber(TeamTwoKills));
}

void UScoreboardWidget::BuildEntriesIfNeeded()
{
	if ( !ScoreboardEntryWidgetClass || !AllyListContainer || !EnemyListContainer )
		return;

	UWorld* World = GetWorld();
	ACGameState* GameState = World ? World->GetGameState<ACGameState>() : nullptr;
	if ( !GameState )
		return;

	uint8 LocalTeamId = 0;
	if ( const ACPlayerState* CPlayerState = Cast<ACPlayerState>(GetOwningPlayerState()) )
	{
		LocalTeamId = CPlayerState->GetTeamIdBasedOnSlot().GetId();
	}

	for ( APlayerState* PS : GameState->PlayerArray )
	{
		ACPlayerState* CPlayerState = Cast<ACPlayerState>(PS);
		if ( !CPlayerState || EntryWidgets.Contains(CPlayerState) )
			continue;

		UScoreboardEntryWidget* NewEntry = CreateWidget<UScoreboardEntryWidget>(this, ScoreboardEntryWidgetClass);
		if ( !NewEntry )
			continue;

		NewEntry->InitializeEntry(CPlayerState);
		EntryWidgets.Add(CPlayerState, NewEntry);

		uint8 EntryTeamId = CPlayerState->GetTeamIdBasedOnSlot().GetId();

		if ( EntryTeamId == LocalTeamId )
			AllyListContainer->AddChild(NewEntry);
		else
			EnemyListContainer->AddChild(NewEntry);
	}
}