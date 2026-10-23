// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/PlayerTeamSlotWidget.h"
#include "Character/PA_CharacterDefinition.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

void UPlayerTeamSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();

	PlayerCharacterIcon->GetDynamicMaterial()->SetScalarParameterValue(CharacterEmptyMatParamName, 1);
	CachedCharacterNameStr = "";
}

void UPlayerTeamSlotWidget::UpdateSlot(const FString& PlayerName, const UPA_CharacterDefinition* CharacterDefinition, bool bIsHost, bool bIsReady)
{
	CachedPlayerNameStr = PlayerName;
	
	if (CharacterDefinition)
	{
		PlayerCharacterIcon->GetDynamicMaterial()->SetTextureParameterValue(CharacterIconMatParamName, CharacterDefinition->LoadIcon());
		PlayerCharacterIcon->GetDynamicMaterial()->SetScalarParameterValue(CharacterEmptyMatParamName, 0);
		CachedCharacterNameStr = CharacterDefinition->GetCharacterDisplayName();
	}
	else
	{
		PlayerCharacterIcon->GetDynamicMaterial()->SetScalarParameterValue(CharacterEmptyMatParamName, 1);
		CachedCharacterNameStr = "";
	}

	UpdateNameText(bIsHost);
	PlayerCharacterIcon->GetDynamicMaterial()->SetScalarParameterValue(IsReadyMatParamName, bIsReady ? 0.4f : 0.f);
}

void UPlayerTeamSlotWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);

	NameText->SetText(FText::FromString(CachedCharacterNameStr));
	PlayAnimationForward(HoverAnim);
}

void UPlayerTeamSlotWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);

	NameText->SetText(FText::FromString(CachedPlayerNameStr));
	PlayAnimationReverse(HoverAnim);
}

void UPlayerTeamSlotWidget::UpdateNameText(bool bIsHost)
{
	FSlateColor TextColor = bIsHost ? FSlateColor(FLinearColor(0.f, 0.5, 1.f)) : FSlateColor(FLinearColor::White);

	if (IsHovered())
	{
		NameText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		NameText->SetText(FText::FromString(CachedCharacterNameStr));
	}
	else
	{
		NameText->SetColorAndOpacity(TextColor);
		NameText->SetText(FText::FromString(CachedPlayerNameStr));
	}
}