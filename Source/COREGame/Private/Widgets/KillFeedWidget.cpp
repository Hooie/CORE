// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/KillFeedWidget.h"
#include "Components/VerticalBox.h"
#include "Framework/CGameState.h"
#include "Widgets/KillFeedEntryWidget.h"

void UKillFeedWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if ( UWorld* World = GetWorld() )
	{
		if ( ACGameState* CGameState = World->GetGameState<ACGameState>() )
		{
			CGameState->OnKillFeedEntryReceived.AddUObject(this, &UKillFeedWidget::OnkillFeedEntryReceived);
		}
	}
}

void UKillFeedWidget::OnkillFeedEntryReceived(const FKillFeedEntry& Entry)
{
	if ( !KillFeedEntryWidgetClass || !KillFeedContainer )
		return;

	UKillFeedEntryWidget* NewEntry = CreateWidget<UKillFeedEntryWidget>(this, KillFeedEntryWidgetClass);
	if ( !NewEntry )
		return;

	NewEntry->ConfigureKillFeedEntry(Entry);
	KillFeedContainer->AddChild(NewEntry);
	NewEntry->PlayAppearAnim();

	TWeakObjectPtr<UKillFeedEntryWidget> WeakEntry(NewEntry);

	FTimerHandle RemoveTimerHandle;
	GetWorld()->GetTimerManager().SetTimer(RemoveTimerHandle, FTimerDelegate::CreateUObject(this, &UKillFeedWidget::RemoveEntry, WeakEntry), EntryDisplayDuration, false);
}

void UKillFeedWidget::RemoveEntry(TWeakObjectPtr<class UKillFeedEntryWidget> EntryWidget)
{
	if ( EntryWidget.IsValid() )
	{
		EntryWidget->PlayVanishAndRemove();
	}
}