// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/GameplayMenu.h"
#include "Components/TextBlock.h"
#include "Components/WidgetSwitcher.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Widgets/GameConfigWidget.h"

void UGameplayMenu::NativeConstruct()
{
	Super::NativeConstruct();

	GameConfigWidgetRoot->OnReturnBtnClicked.AddUObject(this, &UGameplayMenu::ReturnToMenu);
	ConfigBtn->OnClicked.AddDynamic(this, &UGameplayMenu::ConfigBtnClicked);
	QuitMatchBtn->OnClicked.AddDynamic(this, &UGameplayMenu::QuitMatch);
}

FOnButtonClickedEvent& UGameplayMenu::GetResumeButtonClickedEventDelegate()
{
	return ResumeBtn->OnClicked;
}

void UGameplayMenu::SetTitleText(const FString& NewTitleText)
{
	MenuTitle->SetText(FText::FromString(NewTitleText));
}

void UGameplayMenu::ReturnToMenu()
{
	MainSwitcher->SetActiveWidget(MainMenuRoot);
}

void UGameplayMenu::SetResumeButtonVisible(bool bVisible)
{
	if (ResumeBtn)
	{
		ResumeBtn->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

void UGameplayMenu::ConfigBtnClicked()
{
	MainSwitcher->SetActiveWidget(GameConfigWidgetRoot);
}

void UGameplayMenu::QuitMatch()
{
	OnQuitMatchBtnClicked.Broadcast();
}

void UGameplayMenu::QuitGame()
{
	UKismetSystemLibrary::QuitGame(this, GetOwningPlayer(), EQuitPreference::Quit, true);
}