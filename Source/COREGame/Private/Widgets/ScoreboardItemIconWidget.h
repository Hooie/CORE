// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ScoreboardItemIconWidget.generated.h"

/**
 * 
 */
UCLASS()
class UScoreboardItemIconWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	void ConfigureItemSlot(const class UPA_ShopItem* ShopItem, int32 StackCount);

private:
	UPROPERTY(meta = (BindWidget))
	class UImage* IconImage;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* StackCountText;
};
