// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/KillFeedEntryWidget.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/VerticalBox.h"
#include "GenericTeamAgentInterface.h"

namespace
{
	void ApplyKillFeedIcon(UImage* Image, const TSoftObjectPtr<UTexture2D>& Icon)
	{
		if ( !Image )
			return;

		if ( UTexture2D* Texture = Icon.LoadSynchronous() )
		{
			Image->SetBrushFromTexture(Texture);
			Image->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			Image->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}

void UKillFeedEntryWidget::ConfigureKillFeedEntry(const FKillFeedEntry& Entry)
{
	KillerNameText->SetText(FText::FromString(Entry.KillerName));
	VictimNameText->SetText(FText::FromString(Entry.VictimName));


	ApplyKillFeedIcon(KillerIconImage, Entry.KillerIcon);
	ApplyKillFeedIcon(VictimIconImage, Entry.VictimIcon);

	uint8 LocalTeamId = GetLocalTeamId();

	KillerNameText->SetColorAndOpacity(FSlateColor(Entry.KillerTeamId == LocalTeamId ? AllyColor : EnemyColor));
	VictimNameText->SetColorAndOpacity(FSlateColor(Entry.VictimTeamId == LocalTeamId ? AllyColor : EnemyColor));

	if ( AssistIconContainer )
	{
		AssistIconContainer->ClearChildren();

		for ( const TSoftObjectPtr<UTexture2D>& AssistIconSoft : Entry.AssistIcons )
		{
			UTexture2D* AssistTexture = AssistIconSoft.LoadSynchronous();
			if ( !AssistTexture )
				continue;

			UImage* NewIcon = NewObject<UImage>(this);
			NewIcon->SetBrushFromTexture(AssistTexture);
			NewIcon->SetDesiredSizeOverride(AssistSlotOverrideSize);

			AssistIconContainer->AddChildToVerticalBox(NewIcon);
		}
	}
}

void UKillFeedEntryWidget::PlayVanishAndRemove()
{
	if ( VanishAnim )
	{
		FWidgetAnimationDynamicEvent FinishedEvent;
		FinishedEvent.BindDynamic(this, &UKillFeedEntryWidget::OnVanishAnimFinished);
		BindToAnimationFinished(VanishAnim, FinishedEvent);

		PlayAnimationForward(VanishAnim);
	}
	else
	{
		RemoveFromParent();
	}
}

void UKillFeedEntryWidget::PlayAppearAnim()
{
	if ( AppearAnim )
	{
		PlayAnimationForward(AppearAnim);
	}
}

void UKillFeedEntryWidget::OnVanishAnimFinished()
{
	RemoveFromParent();
}

uint8 UKillFeedEntryWidget::GetLocalTeamId() const
{
	if ( const IGenericTeamAgentInterface* TeamAgent = Cast<IGenericTeamAgentInterface>(GetOwningPlayer()) )
	{
		return TeamAgent->GetGenericTeamId().GetId();
	}

	return 0;
}
