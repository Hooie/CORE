// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/ItemStatRow.h"
#include "Components/TextBlock.h"
#include "Inventory/PA_ShopItem.h"

void UItemStatRow::SetStat(const FItemStatEntry& Entry, const FText& Label)
{
	const FString ValueString = Entry.bIsPercent
		? FString::Printf(TEXT("%+.0f%%"), Entry.Magnitude)
		: FString::Printf(TEXT("%+.0f"), Entry.Magnitude);

	if ( ValueText )
	{
		ValueText->SetText(FText::FromString(ValueString));

		if ( const FSlateColor* FoundColor = AttributeValueColors.Find(Entry.Attribute) )
		{
			ValueText->SetColorAndOpacity(*FoundColor);
		}
	}

	if ( AttributeText )
	{
		AttributeText->SetText(Label);
	}

	OnStatSet(Entry.Attribute, Label, Entry.Magnitude, Entry.bIsPercent);
}
