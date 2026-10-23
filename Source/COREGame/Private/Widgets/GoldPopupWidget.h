// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GoldPopupWidget.generated.h"

DECLARE_MULTICAST_DELEGATE(FOnPopupFinished);

/**
 * 
 */
UCLASS()
class UGoldPopupWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	virtual void NativeConstruct() override;

	FOnPopupFinished OnPopupFinished;

	void SetGoldValue(float GoldGained);
	void AddGoldValue(float AdditionalGold);
	void PlayPopup();

private:
	UPROPERTY(meta=(BindWidget))
	class UTextBlock* GoldText;

	UPROPERTY(EditDefaultsOnly, Category = "Visual")
	FLinearColor SpendColor = FLinearColor::White;

	UPROPERTY(EditDefaultsOnly, Category = "Visual")
	FLinearColor GainColor = FLinearColor(1.f, 0.85f, 0.2f);

	UPROPERTY(Transient, meta = (BindWidgetAnim))
	class UWidgetAnimation* PopupAnim;

	float AccumulatedGold = 0.f;

	bool bSuppressFinishedCallback = false;
	
	void UpdateGoldText();

	UFUNCTION()
	void OnPopupAnimFinished();
};
