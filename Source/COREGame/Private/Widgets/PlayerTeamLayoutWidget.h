// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Player/PlayerInfoTypes.h"
#include "PlayerTeamLayoutWidget.generated.h"

class UPlayerTeamSlotWidget;

/**
 * 
 */
UCLASS()
class UPlayerTeamLayoutWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;

	void UpdatePlayerSelection(const TArray<FPlayerSelection>& PlayerSelections);
	
private:
	UPROPERTY(EditDefaultsOnly, Category = "Visual")
	float PlayerTeamWidgetSlotMargin = 5.f;

	UPROPERTY(EditDefaultsOnly, Category = "Visual")
	TSubclassOf<UPlayerTeamSlotWidget> PlayerTeamSlotWidgetClass;

	UPROPERTY(meta=(BindWidget))
	class UHorizontalBox* TeamOneLayOutBox;

	UPROPERTY(meta = (BindWidget))
	class UHorizontalBox* TeamTwoLayOutBox;

	UPROPERTY()
	TArray<UPlayerTeamSlotWidget*> TeamSlotWidgets;
};
