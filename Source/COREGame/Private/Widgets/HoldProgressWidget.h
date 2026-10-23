// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HoldProgressWidget.generated.h"

/**
 * 
 */
UCLASS()
class UHoldProgressWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:
	virtual void NativeConstruct() override;

	void StartHold(float InMaxHoldTime, bool bCountDown = false);
	void StopHold();
	
protected:

private:
	UPROPERTY(meta = (BindWidget))
	class UProgressBar* HoldProgressBar;

	UPROPERTY(meta = (BindWidget))
	class UTextBlock* HoldTimeText;

	FNumberFormattingOptions FormatOps;

	float MaxHoldTime = 0.f;
	float StartTime = 0.f;

	bool bIsCountDown = false;

	FTimerHandle UpdateTimerHandle;
	void UpdateProgress();
};
