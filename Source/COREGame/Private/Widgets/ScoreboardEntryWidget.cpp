// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/ScoreboardEntryWidget.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Character/PA_CharacterDefinition.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/HorizontalBox.h"
#include "EngineUtils.h"
#include "GAS/CHeroAttributeSet.h"
#include "Player/CPlayerCharacter.h"
#include "Player/CPlayerState.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/InventoryItem.h"
#include "Widgets/ScoreboardItemIconWidget.h"

void UScoreboardEntryWidget::InitializeEntry(ACPlayerState* InPlayerState)
{
	PlayerState = InPlayerState;
	if ( !PlayerState )
		return;

	const FPlayerSelection& Selection = PlayerState->GetPlayerSelection();
	PlayerNicknameText->SetText(FText::FromString(Selection.GetPlayerNickName()));

	if ( const UPA_CharacterDefinition* Definition = Selection.GetCharacterDefinition() )
	{
		if ( UTexture2D* Icon = Definition->LoadIcon() )
		{
			HeroIconImage->SetBrushFromTexture(Icon);
		}
	}

	RefreshStats();
}

void UScoreboardEntryWidget::RefreshStats()
{
	if ( !PlayerState )
		return;

	KillCountText->SetText(FText::AsNumber(PlayerState->GetKills()));
	DeathCountText->SetText(FText::AsNumber(PlayerState->GetDeath()));
	AssistCountText->SetText(FText::AsNumber(PlayerState->GetAssists()));
	MinionKillCountText->SetText(FText::AsNumber(PlayerState->GetMinionKills()));

	if ( PlayerLevelText )
	{
		ACPlayerCharacter* PlayerCharacter = nullptr;

		if ( UWorld* World = GetWorld() )
		{
			for ( TActorIterator<ACPlayerCharacter> It(World); It; ++It )
			{
				if ( It->GetPlayerState<ACPlayerState>() == PlayerState )
				{
					PlayerCharacter = *It;
					break;
				}
			}
		}

		if ( PlayerCharacter )
		{
			if ( UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(PlayerCharacter) )
			{
				bool bFound = false;
				float Level = ASC->GetGameplayAttributeValue(UCHeroAttributeSet::GetLevelAttribute(), bFound);

				if ( bFound )
				{
					PlayerLevelText->SetText(FText::AsNumber(( int ) Level));
				}
			}
		}
	}

	if ( ItemIconContainer && ItemWidgetClass )
	{
		ItemIconContainer->ClearChildren();

		for ( const FScoreboardItemEntry& Entry : PlayerState->GetScoreboardItems() )
		{
			if ( !Entry.ShopItem )
				continue;

			UScoreboardItemIconWidget* NewIcon = CreateWidget<UScoreboardItemIconWidget>(this, ItemWidgetClass);
			if ( !NewIcon )
				continue;

			NewIcon->ConfigureItemSlot(Entry.ShopItem, Entry.StackCount);
			NewIcon->SetRenderOpacity(1.f);
			NewIcon->SetColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, 1.f));
			NewIcon->SetPadding(FMargin(0.f, 0.f, 2.f, 0.f));

			ItemIconContainer->AddChild(NewIcon);
		}
	}
}