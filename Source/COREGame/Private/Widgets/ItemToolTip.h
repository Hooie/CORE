// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ItemToolTip.generated.h"

class UPA_ShopItem;

/**
 * 
 */
UCLASS()
class UItemToolTip : public UUserWidget
{
	GENERATED_BODY()
	
public:
	void SetItem(const UPA_ShopItem* Item);
	void SetPrice(float NewPrice);
	void SetPurchasePrice(float EffectivePrice, float OriginalPrice);

private:
	UPROPERTY(meta=(BindWidget))
	class UImage* IconImage;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* ItemTitleText;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* ItemDescriptionText;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* ItemPriceText;

	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* OriginalPriceText;

	UPROPERTY(meta = (BindWidgetOptional))
	class UWidget* OriginalPricePanel;

	void SetOriginalPriceVisible(bool bVisible);

	UPROPERTY(meta = (BindWidget))
	class UVerticalBox* StatsBox;

	void BuildStatEntries(const UPA_ShopItem* Item);

	UPROPERTY(EditDefaultsOnly, Category = "Stats")
	TSubclassOf<class UItemStatRow> StatRowClass;

	UPROPERTY(EditDefaultsOnly, Category = "Stats")
	FMargin StatRowPadding = FMargin(0.f, 2.f);
};
