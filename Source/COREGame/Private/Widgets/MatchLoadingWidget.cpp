// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/MatchLoadingWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Framework/CGameState.h"
#include "Framework/CLoadingScreenSubsystem.h"
#include "Network/CNetStatics.h"
#include "Player/CPlayerController.h"
#include "Player/CPlayerState.h"
#include "Widgets/MatchLoadingPlayerSlot.h"

void UMatchLoadingWidget::NativeConstruct()
{
	Super::NativeConstruct();

	BuildSlots();
	Refresh();
}

void UMatchLoadingWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	DisplayedLocalProgress = FMath::FInterpTo(DisplayedLocalProgress, TargetLocalProgress, InDeltaTime, ProgressBarInterpSpeed);

	if ( LoadingProgressBar )
	{
		LoadingProgressBar->SetPercent(DisplayedLocalProgress);
	}

	if ( LoadingProgressText )
	{
		LoadingProgressText->SetText(FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(DisplayedLocalProgress * 100.f))));
	}

	RefreshElapsed += InDeltaTime;
	if ( RefreshElapsed < RefreshInterval )
		return;

	RefreshElapsed = 0.f;
	Refresh();
}

void UMatchLoadingWidget::BuildSlots()
{
	BlueSlots.Reset();
	RedSlots.Reset();
	BlueTeamList->ClearChildren();
	RedTeamList->ClearChildren();

	const int32 TeamSize = UCNetStatics::GetPlayerCountPerTeam();

	for ( int32 i = 0; i < TeamSize; ++i )
	{
		BlueSlots.Add(CreateTeamSlot(BlueSlotClass, BlueTeamList));
		RedSlots.Add(CreateTeamSlot(RedSlotClass, RedTeamList));
	}
}

UMatchLoadingPlayerSlot* UMatchLoadingWidget::CreateTeamSlot(TSubclassOf<UMatchLoadingPlayerSlot> SlotClass, UVerticalBox* TeamList)
{
	if ( !SlotClass || !TeamList )
		return nullptr;

	UMatchLoadingPlayerSlot* NewSlot = CreateWidget<UMatchLoadingPlayerSlot>(this, SlotClass);
	if ( !NewSlot )
		return nullptr;

	if ( UVerticalBoxSlot* BoxSlot = TeamList->AddChildToVerticalBox(NewSlot) )
	{
		BoxSlot->SetPadding(SlotPadding);
	}

	NewSlot->SetEmpty();
	return NewSlot;
}

const ACPlayerState* UMatchLoadingWidget::FindLivePlayerState(const ACGameState* GameState, const FPlayerSelection& Selection)
{
	if ( !GameState )
		return nullptr;

	for ( APlayerState* PS : GameState->PlayerArray )
	{
		const ACPlayerState* CPS = Cast<ACPlayerState>(PS);
		if ( CPS && CPS->GetPlayerSelection().GetPlayerSlot() == Selection.GetPlayerSlot() )
		{
			return CPS;
		}
	}

	return nullptr;
}

void UMatchLoadingWidget::Refresh()
{
	if ( bHideRequested )
		return;

	UGameInstance* GameInstance = GetGameInstance();
	UWorld* World = GetWorld();
	UCLoadingScreenSubsystem* LoadingSubsystem = GameInstance ? GameInstance->GetSubsystem<UCLoadingScreenSubsystem>() : nullptr;

	if ( !World || !LoadingSubsystem )
		return;

	const bool bInGameLevel = LoadingSubsystem->IsInGameLevel(World);
	const ACGameState* CGameState = bInGameLevel ? World->GetGameState<ACGameState>() : nullptr;
	ACPlayerController* LocalPC = bInGameLevel ? Cast<ACPlayerController>(GameInstance->GetFirstLocalPlayerController(World)) : nullptr;
	const ACPlayerState* LocalPS = LocalPC ? LocalPC->GetPlayerState<ACPlayerState>() : nullptr;
	const bool bLocalReady = LocalPC && LocalPC->IsClientReadyReported();

	const int32 LocalProgress = bLocalReady ? 100 : (bInGameLevel ? (LocalPC ? LocalPC->GetLocalLoadingProgress() : 70) : FMath::FloorToInt(LoadingSubsystem->GetGameLevelLoadProgress() * 70.f));
	
	TargetLocalProgress = FMath::Clamp(LocalProgress / 100.f, 0.f, 1.f);

	TArray<FPlayerSelection> Roster = LoadingSubsystem->GetRosterSnapshot();
	if ( Roster.Num() == 0 && CGameState )
	{
		for ( APlayerState* PS : CGameState->PlayerArray )
		{
			if ( const ACPlayerState* CPS = Cast<ACPlayerState>(PS) )
			{
				Roster.Add(CPS->GetPlayerSelection());
			}
		}
	}

	const int32 TeamSize = BlueSlots.Num();
	TArray<bool> BlueFilled;
	TArray<bool> RedFilled;
	BlueFilled.Init(false, TeamSize);
	RedFilled.Init(false, TeamSize);

	for ( const FPlayerSelection& Selection : Roster )
	{
		const uint8 PlayerSlot = Selection.GetPlayerSlot();
		if ( PlayerSlot == FPlayerSelection::GetInvalidSlot() )
			continue;

		const bool bBlue = PlayerSlot < TeamSize;
		const int32 Index = bBlue ? PlayerSlot : PlayerSlot - TeamSize;
		TArray<UMatchLoadingPlayerSlot*>& Slots = bBlue ? BlueSlots : RedSlots;

		if ( !Slots.IsValidIndex(Index) || !Slots[ Index ] )
			continue;

		const ACPlayerState* LivePS = FindLivePlayerState(CGameState, Selection);
		const bool bIsMe = LivePS ? (LivePS == LocalPS) : LoadingSubsystem->IsLocalPlayerSelection(Selection);

		int32 Progress = -1;
		bool bReady = false;

		if ( bIsMe )
		{
			Progress = LocalProgress;
			bReady = bLocalReady;
		}
		else if ( LivePS )
		{
			Progress = LivePS->GetLoadingProgress();
			bReady = LivePS->IsLoadingComplete();
		}

		Slots[ Index ]->SetPlayer(Selection, LivePS ? LivePS->GetPlayerName() : FString(), Progress, bReady);
		(bBlue ? BlueFilled : RedFilled)[ Index ] = true;
	}

	for ( int32 i = 0; i < TeamSize; ++i )
	{
		if ( !BlueFilled[ i ] && BlueSlots[ i ] ) BlueSlots[ i ]->SetEmpty();
		if ( !RedFilled[ i ] && RedSlots[ i ] ) RedSlots[ i ]->SetEmpty();
	}

	if ( bLocalReady && CGameState && CGameState->GetMatchPhase() != EMatchPhase::WaitingForPlayers )
	{
		bHideRequested = true;
		LoadingSubsystem->RequestHideLoadingScreen();
		return;
	}

	if ( StatusText )
	{
		StatusText->SetText(bLocalReady ? WaitingPlayersText : LoadingResourcesText);
	}
}