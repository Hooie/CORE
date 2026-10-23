// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/ItemWidget.h"
#include "ShopItemIconWidget.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnIconClicked, const UPA_ShopItem*);


/**
 * 
 */
UCLASS()
class UShopItemIconWidget : public UItemWidget
{
	GENERATED_BODY()
	
public:
	FOnIconClicked OnIconClicked;

	void InitWithShopItem(const UPA_ShopItem* NewShopItem);

private:
	UPROPERTY()
	const UPA_ShopItem* ShopItem;

	virtual void LeftButtonClicked() override;
	
};
