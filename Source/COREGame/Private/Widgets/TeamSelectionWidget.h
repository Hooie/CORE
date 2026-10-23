// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TeamSelectionWidget.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnSlotClicked, uint8 /*Slot ID*/);

/**
 * 
 */
UCLASS()
class UTeamSelectionWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	FOnSlotClicked OnSlotClicked;

	virtual void NativeConstruct() override;
	
	void SetSlotID(uint8 NewSlotID);
	void UpdateSlotInfo(const FString& PlayerNickName, bool bIsHost = false, bool bIsReady = false);
	
	void SetSelectButtonEnabled(bool bEnabled);
		
private:
	UPROPERTY(meta = (BindWidget))
	class UButton* SelectButton;
	
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* InfoText;

	UFUNCTION()
	void SelectButtonClicked();

	uint8 SlotID;
};
