// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Player/PlayerInfoTypes.h"
#include "MatchLoadingPlayerSlot.generated.h"

class ACPlayerState;
class UPA_CharacterDefinition;

/**
 * 
 */
UCLASS()
class UMatchLoadingPlayerSlot : public UUserWidget
{
	GENERATED_BODY()
	
public:
	void SetPlayer(const FPlayerSelection& Selection, const FString& FallbackName, int32 Progress, bool bReady);
	void SetEmpty();
	
private:
	UPROPERTY(meta = (BindWidget))
	class UImage* HeroIcon;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* NickNameText;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* ProgressText;

	UPROPERTY(meta = (BindWidgetOptional))
	class UProgressBar* ProgressBar;

	UPROPERTY(EditDefaultsOnly, Category = "Visual")
	FSlateColor LoadingColor = FSlateColor(FLinearColor(0.75f, 0.75f, 0.75f));

	UPROPERTY(EditDefaultsOnly, Category = "Visual")
	FSlateColor ReadyColor = FSlateColor(FLinearColor(0.45f, 1.f, 0.55f));

	UPROPERTY(EditDefaultsOnly, Category = "Visual")
	FText UnknownProgressText = INVTEXT("...");

	UPROPERTY()
	const UPA_CharacterDefinition* CachedDefinition = nullptr;
};
