// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ScoreboardEntryWidget.generated.h"

/**
 * 
 */
UCLASS()
class UScoreboardEntryWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	void InitializeEntry(class ACPlayerState* InPlayerState);
	void RefreshStats();

private:
	UPROPERTY()
	class ACPlayerState* PlayerState;

	UPROPERTY(meta = (BindWidget))
	class UImage* HeroIconImage;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* PlayerNicknameText;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* PlayerLevelText;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* KillCountText;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* DeathCountText;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* AssistCountText;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* MinionKillCountText;

	UPROPERTY(meta = (BindWidget))
	class UHorizontalBox* ItemIconContainer;

	UPROPERTY(EditDefaultsOnly, Category = "Scoreboard")
	TSubclassOf<class UScoreboardItemIconWidget> ItemWidgetClass;
};
