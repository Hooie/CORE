// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Player/PlayerInfoTypes.h"
#include "MatchLoadingWidget.generated.h"

class UMatchLoadingPlayerSlot;
class ACGameState;
class ACPlayerState;

/**
 * 
 */
UCLASS()
class UMatchLoadingWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	UPROPERTY(meta = (BindWidget))
	class UVerticalBox* BlueTeamList;

	UPROPERTY(meta = (BindWidget))
	class UVerticalBox* RedTeamList;

	UPROPERTY(meta = (BindWidgetOptional))
	class UTextBlock* StatusText;

	UPROPERTY(meta = (BindWidget))
	class UProgressBar* LoadingProgressBar;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* LoadingProgressText;

	UPROPERTY(EditDefaultsOnly, Category = "Loading")
	TSubclassOf<UMatchLoadingPlayerSlot> BlueSlotClass;

	UPROPERTY(EditDefaultsOnly, Category = "Loading")
	TSubclassOf<UMatchLoadingPlayerSlot> RedSlotClass;

	UPROPERTY(EditDefaultsOnly, Category = "Loading")
	FMargin SlotPadding = FMargin(0.f, 0.f, 0.f, 12.f);

	UPROPERTY(EditDefaultsOnly, Category = "Loading")
	float RefreshInterval = 0.1f;

	UPROPERTY(EditDefaultsOnly, Category = "Loading")
	FText LoadingResourcesText = INVTEXT("리소스를 불러오는 중...");

	UPROPERTY(EditDefaultsOnly, Category = "Loading")
	FText WaitingPlayersText = INVTEXT("다른 플레이어를 기다리는 중...");

	UPROPERTY(EditDefaultsOnly, Category = "Loading")
	float ProgressBarInterpSpeed = 3.f;

	UPROPERTY()
	TArray<UMatchLoadingPlayerSlot*> BlueSlots;

	UPROPERTY()
	TArray<UMatchLoadingPlayerSlot*> RedSlots;

	float RefreshElapsed = 0.f;
	bool bHideRequested = false;

	float TargetLocalProgress = 0.f;
	float DisplayedLocalProgress = 0.f;

	void BuildSlots();
	UMatchLoadingPlayerSlot* CreateTeamSlot(TSubclassOf<UMatchLoadingPlayerSlot> SlotClass, class UVerticalBox* TeamList);
	void Refresh();

	static const ACPlayerState* FindLivePlayerState(const ACGameState* GameState, const FPlayerSelection& Selection);
};
