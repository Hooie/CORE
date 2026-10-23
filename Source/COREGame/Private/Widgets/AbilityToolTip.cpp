// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/AbilityToolTip.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "AbilityToolTip.h"

void UAbilityToolTip::SetAbilityInfo(const FName& AbilityName, UTexture2D* AbilityTexture, const FText& AbilityDescription, float AbilityCooldown, float AbilityCost)
{
	AbilityNameText->SetText(FText::FromName(AbilityName));
	AbilityIcon->SetBrushFromTexture(AbilityTexture);
	
	AbilityDescriptionText->SetText(AbilityDescription);

	FNumberFormattingOptions FormattingOptions;
	FormattingOptions.MaximumFractionalDigits = 0;

	if ( AbilityCooldown <= 0.f )
	{
		AbilityCooldownLabel->SetText(FText::FromString(TEXT("패시브")));
		AbilityCooldownText->SetVisibility(ESlateVisibility::Hidden);
	}
	AbilityCooldownText->SetText(FText::AsNumber(AbilityCooldown, &FormattingOptions));

	if ( AbilityCost <= 0.f )
	{
		AbilityCostLabel->SetVisibility(ESlateVisibility::Hidden);
		AbilityCostText->SetVisibility(ESlateVisibility::Hidden);
	}
	AbilityCostText->SetText(FText::AsNumber(AbilityCost, &FormattingOptions));
}
