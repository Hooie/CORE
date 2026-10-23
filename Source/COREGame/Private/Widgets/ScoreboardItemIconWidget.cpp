// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/ScoreboardItemIconWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Inventory/PA_ShopItem.h"

void UScoreboardItemIconWidget::ConfigureItemSlot(const UPA_ShopItem* ShopItem, int32 StackCount)
{
	if ( !ShopItem )
		return;

	if ( UTexture2D* Icon = ShopItem->GetIcon() )
	{
		IconImage->SetBrushFromTexture(Icon);

		IconImage->SetColorAndOpacity(FLinearColor::White);
		IconImage->SetBrushTintColor(FSlateColor(FLinearColor::White));
		IconImage->SetRenderOpacity(1.f);
	}

	if ( StackCountText )
	{
		StackCountText->SetText(StackCount > 1 ? FText::AsNumber(StackCount) : FText::GetEmpty());
	}
}
