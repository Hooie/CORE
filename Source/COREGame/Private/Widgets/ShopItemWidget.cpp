// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/ShopItemWidget.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/ListView.h"
#include "Components/TextBlock.h"
#include "Framework/CAssetManager.h"
#include "GameFramework/Pawn.h"
#include "GAS/CHeroAttributeSet.h"
#include "GAS/CAbilitySystemStatics.h"
#include "Inventory/PA_ShopItem.h"
#include "Inventory/InventoryComponent.h"
#include "Widgets/ItemToolTip.h"

UUserWidget* UShopItemWidget::GetWidget() const
{
	UShopItemWidget* Copy = CreateWidget<UShopItemWidget>(GetOwningPlayer(), GetClass());
	
	Copy->CopyFromOther(this);

	return Copy;
}

TArray<const ITreeNodeInterface*> UShopItemWidget::GetInputs() const
{
	const FItemCollection* Collection = UCAssetManager::Get().GetCombinationForItem(GetShopItem());

	if (Collection)
	{
		return ItemsToInterfaces(Collection->GetItems());
	}

	return TArray<const ITreeNodeInterface*>{};
}

TArray<const ITreeNodeInterface*> UShopItemWidget::GetOutputs() const
{
	const FItemCollection* Collection = UCAssetManager::Get().GetIngredientForItem(GetShopItem());

	if (Collection)
	{
		return ItemsToInterfaces(Collection->GetItems());
	}

	return TArray<const ITreeNodeInterface*>{};
}

const UObject* UShopItemWidget::GetItemObject() const
{
	return ShopItem;
}

void UShopItemWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	IUserObjectListEntry::NativeOnListItemObjectSet(ListItemObject);
	InitWithShopItem(Cast<UPA_ShopItem>(ListItemObject));
	ParentListView = Cast<UListView>(IUserListEntry::GetOwningListView());
}

void UShopItemWidget::CopyFromOther(const UShopItemWidget* OtherWidget)
{
	OnItemPurchaseIssued = OtherWidget->OnItemPurchaseIssued;
	OnShopItemClicked = OtherWidget->OnShopItemClicked;
	ParentListView = OtherWidget->ParentListView;
	InitWithShopItem(OtherWidget->GetShopItem());
}

void UShopItemWidget::InitWithShopItem(const UPA_ShopItem* NewShopItem)
{
	ShopItem = NewShopItem;
	if (!ShopItem)
		return;

	SetIcon(ShopItem->GetIcon());

	CachedToolTip = SetToolTipWidget(ShopItem);

	RefreshState();
}

TArray<const ITreeNodeInterface*> UShopItemWidget::ItemsToInterfaces(const TArray<const UPA_ShopItem*>& Items) const
{
	TArray<const ITreeNodeInterface*> RetInterfaces;

	for ( const UPA_ShopItem* Item : Items )
	{
		const UShopItemWidget* ItemWidget = ParentListView ? ParentListView->GetEntryWidgetFromItem<UShopItemWidget>(Item) : nullptr;

		if ( !ItemWidget )
		{
			UShopItemWidget* DetachedWidget = CreateWidget<UShopItemWidget>(GetOwningPlayer(), GetClass());
			if ( DetachedWidget )
			{
				DetachedWidget->ParentListView = ParentListView;
				DetachedWidget->OnItemPurchaseIssued = OnItemPurchaseIssued;
				DetachedWidget->OnShopItemClicked = OnShopItemClicked;
				DetachedWidget->InitWithShopItem(Item);
				ItemWidget = DetachedWidget;
			}
		}

		if ( ItemWidget )
		{
			RetInterfaces.Add(ItemWidget);
		}
	}

	return RetInterfaces;
}

void UShopItemWidget::RightButtonClicked()
{
	OnItemPurchaseIssued.Broadcast(GetShopItem());
}

void UShopItemWidget::LeftButtonClicked()
{
	OnShopItemClicked.Broadcast(this);
}

UInventoryComponent* UShopItemWidget::GetOwnerInventory() const
{
	APawn* OwnerPawn = GetOwningPlayerPawn();

	return OwnerPawn ? OwnerPawn->FindComponentByClass<UInventoryComponent>() : nullptr;
}

void UShopItemWidget::RefreshState()
{
	if ( !ShopItem )
		return;

	UInventoryComponent* InventoryComponent = GetOwnerInventory();

	const float EffectivePrice = InventoryComponent ? InventoryComponent->GetEffectivePrice(ShopItem) : ShopItem->GetPrice();
	if ( PriceText )
	{
		PriceText->SetText(FText::AsNumber(FMath::RoundToInt(EffectivePrice)));
	}

	if ( CachedToolTip )
	{
		CachedToolTip->SetPurchasePrice(EffectivePrice, ShopItem->GetPrice());
	}

	if ( !InventoryComponent )
		return;

	const bool bOwned = InventoryComponent->TryGetItemForShopItem(ShopItem) != nullptr;
	const bool bEnoughGold = InventoryComponent->GetGold() >= EffectivePrice;
	const bool bAffordable = InventoryComponent->EvaluatePurchase(ShopItem, false) == EPurchaseResult::Success;

	if ( UImage* Icon = GetItemIcon() )
		Icon->SetColorAndOpacity(bAffordable ? FLinearColor::White : UnaffordableIconColor);

	if ( PriceText )
		PriceText->SetColorAndOpacity(bEnoughGold ? AffordablePriceColor : UnaffordablePriceColor);

	if ( OwnedHighlightBorder )
		OwnedHighlightBorder->SetVisibility(bOwned ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}

void UShopItemWidget::ApplyTreeOwnership(bool bShowOwned, bool bDimmed)
{
	RefreshState();

	if ( OwnedHighlightBorder )
		OwnedHighlightBorder->SetVisibility(bShowOwned ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);

	if ( bDimmed )
	{
		if ( UImage* Icon = GetItemIcon() )
			Icon->SetColorAndOpacity(UnaffordableIconColor);
	}
}
