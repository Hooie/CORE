// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/MatchLoadingPlayerSlot.h"
#include "Character/PA_CharacterDefinition.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/ProgressBar.h"
#include "Player/CPlayerState.h"

void UMatchLoadingPlayerSlot::SetPlayer(const FPlayerSelection& Selection, const FString& FallbackName, int32 Progress, bool bReady)
{
	const UPA_CharacterDefinition* Definition = Selection.GetCharacterDefinition();
	if ( Definition != CachedDefinition )
	{
		CachedDefinition = Definition;

		UTexture2D* Icon = Definition ? Definition->LoadIcon() : nullptr;
		if ( Icon )
		{
			HeroIcon->SetBrushFromTexture(Icon);
			HeroIcon->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			HeroIcon->SetVisibility(ESlateVisibility::Hidden);
		}
	}

	NickNameText->SetVisibility(ESlateVisibility::HitTestInvisible);
	ProgressText->SetVisibility(ESlateVisibility::HitTestInvisible);

	FString NickName = Selection.GetPlayerNickName();
	if ( NickName.IsEmpty() )
	{
		NickName = FallbackName;
	}
	NickNameText->SetText(FText::FromString(NickName));

	if ( bReady )
	{
		Progress = 100;
	}

	ProgressText->SetText(Progress >= 0 ? FText::FromString(FString::Printf(TEXT("%d%%"), Progress)) : UnknownProgressText);
	ProgressText->SetColorAndOpacity(bReady ? ReadyColor : LoadingColor);

	if ( ProgressBar )
	{
		ProgressBar->SetVisibility(ESlateVisibility::HitTestInvisible);
		ProgressBar->SetPercent(FMath::Max(0, Progress) / 100.f);
	}
}

void UMatchLoadingPlayerSlot::SetEmpty()
{
	CachedDefinition = nullptr;

	HeroIcon->SetVisibility(ESlateVisibility::Hidden);
	NickNameText->SetVisibility(ESlateVisibility::Hidden);
	ProgressText->SetVisibility(ESlateVisibility::Hidden);

	if ( ProgressBar )
	{
		ProgressBar->SetPercent(0.f);
		ProgressBar->SetVisibility(ESlateVisibility::Hidden);
	}
}
