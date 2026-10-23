// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "GameplayMenu.generated.h"

DECLARE_MULTICAST_DELEGATE(FOnQuitMatchBtnClicked);

class UTextBlock;

/**
 * 
 */
UCLASS()
class UGameplayMenu : public UUserWidget
{
	GENERATED_BODY()

public:
	FOnQuitMatchBtnClicked OnQuitMatchBtnClicked;

	virtual void NativeConstruct() override;

	FOnButtonClickedEvent& GetResumeButtonClickedEventDelegate();

	void SetTitleText(const FString& NewTitleText);

	void ReturnToMenu();

	void SetResumeButtonVisible(bool bVisible);

private:
	UPROPERTY(meta=(BindWidget))
	class UWidgetSwitcher* MainSwitcher;

	UPROPERTY(meta = (BindWidget))
	UWidget* MainMenuRoot;

	UPROPERTY(meta=(BindWidget))
	UTextBlock* MenuTitle;

	UPROPERTY(meta = (BindWidget))
	UButton* ResumeBtn;

	UPROPERTY(meta = (BindWidget))
	UButton* ConfigBtn;

	UPROPERTY(meta = (BindWidget))
	UButton* QuitMatchBtn;

	UPROPERTY(meta = (BindWidget))
	class UGameConfigWidget* GameConfigWidgetRoot;


	UFUNCTION()
	void ConfigBtnClicked();

	UFUNCTION()
	void QuitMatch();

	UFUNCTION()
	void QuitGame();
};
