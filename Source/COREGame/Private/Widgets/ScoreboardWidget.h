// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ScoreboardWidget.generated.h"

/**
 * 
 */
UCLASS()
class UScoreboardWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	void RefreshScoreboard();

private:
	UPROPERTY(meta = (BindWidget))
	class UVerticalBox* AllyListContainer;

	UPROPERTY(meta = (BindWidget))
	class UVerticalBox* EnemyListContainer;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* TeamOneTotalKillCountText;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* TeamTwoTotalKillCountText;
	
	UPROPERTY(EditDefaultsOnly, Category = "Scoreboard")
	TSubclassOf<class UScoreboardEntryWidget> ScoreboardEntryWidgetClass;

	UPROPERTY()
	TMap<class ACPlayerState*, class UScoreboardEntryWidget*> EntryWidgets;

	void BuildEntriesIfNeeded();
};
