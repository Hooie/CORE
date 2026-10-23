// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/TeamSelectionWidget.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"

void UTeamSelectionWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SelectButton->OnClicked.AddDynamic(this, &UTeamSelectionWidget::SelectButtonClicked);
}

void UTeamSelectionWidget::SetSlotID(uint8 NewSlotID)
{
	SlotID = NewSlotID;
}

void UTeamSelectionWidget::UpdateSlotInfo(const FString& PlayerNickName, bool bIsHost, bool bIsReady)
{
	FSlateColor TextColor;

	if ( bIsHost )
	{
		TextColor = FSlateColor(FLinearColor(0.f, 0.5f, 1.f));
	}
	else if ( bIsReady )
	{
		TextColor = FSlateColor(FLinearColor::Green);
	}
	else
	{
		TextColor = FSlateColor(FLinearColor::White);
	}
	
	InfoText->SetColorAndOpacity(TextColor);
	InfoText->SetText(FText::FromString(PlayerNickName));
}

void UTeamSelectionWidget::SetSelectButtonEnabled(bool bEnabled)
{
	SelectButton->SetIsEnabled(bEnabled);
}

void UTeamSelectionWidget::SelectButtonClicked()
{
	OnSlotClicked.Broadcast(SlotID);
}
