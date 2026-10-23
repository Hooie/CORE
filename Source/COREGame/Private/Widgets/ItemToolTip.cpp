// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/ItemToolTip.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "GAS/CGameplayEffectSettings.h"
#include "Inventory/PA_ShopItem.h"
#include "Widgets/ItemStatRow.h"

void UItemToolTip::SetItem(const UPA_ShopItem* Item)
{
	ItemTitleText->SetText(Item->GetItemName());
	ItemDescriptionText->SetText(Item->GetItemDescription());
	SetPurchasePrice(Item->GetPrice(), Item->GetPrice());
	IconImage->SetBrushFromTexture(Item->GetIcon());

	BuildStatEntries(Item);
}

void UItemToolTip::SetPrice(float NewPrice)
{
	ItemPriceText->SetText(FText::AsNumber(( int ) NewPrice));
	SetOriginalPriceVisible(false);
}

void UItemToolTip::SetPurchasePrice(float EffectivePrice, float OriginalPrice)
{
	ItemPriceText->SetText(FText::AsNumber(FMath::RoundToInt(EffectivePrice)));

	const bool bDiscounted = FMath::RoundToInt(EffectivePrice) < FMath::RoundToInt(OriginalPrice);

	if ( OriginalPriceText )
	{
		OriginalPriceText->SetText(FText::AsNumber(FMath::RoundToInt(OriginalPrice)));
	}

	SetOriginalPriceVisible(bDiscounted);
}

void UItemToolTip::SetOriginalPriceVisible(bool bVisible)
{
	UWidget* Target = OriginalPricePanel ? OriginalPricePanel : OriginalPriceText;
	if ( Target )
	{
		Target->SetVisibility(bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UItemToolTip::BuildStatEntries(const UPA_ShopItem* Item)
{
	if ( !StatsBox )
		return;

	StatsBox->ClearChildren();

	if ( !StatRowClass )
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: StatRowClass가 지정되지 않았습니다. 툴팁 WBP의 Class Defaults에서 설정하세요."), *GetName());
		StatsBox->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	const UCGameplayEffectSettings* Settings = GetDefault<UCGameplayEffectSettings>();

	for ( const FItemStatEntry& Entry : Item->GetStatEntries() )
	{
		FText AttributeLabel;

		if ( Settings )
		{
			if ( Entry.bIsPercent )
			{
				if ( const FText* FoundLabel = Settings->PercentAttributeDisplayNames.Find(Entry.Attribute) )
				{
					AttributeLabel = *FoundLabel;
				}
			}

			if ( AttributeLabel.IsEmpty() )
			{
				if ( const FText* FoundLabel = Settings->AttributeDisplayNames.Find(Entry.Attribute) )
				{
					AttributeLabel = *FoundLabel;
				}
			}
		}

		if ( AttributeLabel.IsEmpty() )
		{
			AttributeLabel = FText::FromString(Entry.Attribute.GetName());
		}

		if ( AttributeLabel.IsEmpty() )
		{
			AttributeLabel = FText::FromString(Entry.Attribute.GetName());
		}

		UItemStatRow* Row = CreateWidget<UItemStatRow>(this, StatRowClass);
		if ( !Row )
			continue;

		Row->SetStat(Entry, AttributeLabel);

		if ( UVerticalBoxSlot* RowSlot = StatsBox->AddChildToVerticalBox(Row) )
		{
			RowSlot->SetPadding(StatRowPadding);
			RowSlot->SetHorizontalAlignment(HAlign_Fill);
		}
	}

	StatsBox->SetVisibility(StatsBox->GetChildrenCount() > 0 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}