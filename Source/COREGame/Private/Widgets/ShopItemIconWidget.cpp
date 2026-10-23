// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/ShopItemIconWidget.h"
#include "Inventory/PA_ShopItem.h"

void UShopItemIconWidget::InitWithShopItem(const UPA_ShopItem* NewShopItem)
{
	ShopItem = NewShopItem;
	if ( !ShopItem )
		return;

	SetIcon(ShopItem->GetIcon());
	SetToolTipWidget(ShopItem);
}

void UShopItemIconWidget::LeftButtonClicked()
{
	OnIconClicked.Broadcast(ShopItem);

}