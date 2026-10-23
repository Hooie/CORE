// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/ShopCategoryButton.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

void UShopCategoryButton::NativeConstruct()
{
	Super::NativeConstruct();

	if ( CategoryButton )
	{
		CategoryButton->OnClicked.AddDynamic(this, &UShopCategoryButton::HandleClicked);
	}
}

void UShopCategoryButton::InitCategory(const FGameplayTag& InTag, const FText& InDisplayName, UTexture2D* InIcon)
{
	CategoryTag = InTag;

	if ( CategoryNameText )
	{
		CategoryNameText->SetText(InDisplayName);
	}

	if( CategoryNameText )
	{
		if ( InIcon )
		{
			CategoryIcon->SetBrushFromTexture(InIcon);
			CategoryIcon->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			CategoryIcon->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	SetSelected(false);
}

void UShopCategoryButton::SetSelected(bool bSelected)
{
	if ( CategoryButton )
	{
		CategoryButton->SetBackgroundColor(bSelected ? SelectedColor : NormalColor);
	}
}

void UShopCategoryButton::HandleClicked()
{
	OnCategoryBtnClicked.Broadcast(CategoryTag);
}
