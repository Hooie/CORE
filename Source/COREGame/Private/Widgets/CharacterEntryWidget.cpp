// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/CharacterEntryWidget.h"
#include "Character/PA_CharacterDefinition.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

void UCharacterEntryWidget::NativeOnListItemObjectSet(UObject* ListItemObject)
{
	IUserObjectListEntry::NativeOnListItemObjectSet(ListItemObject);
	
	CharacterDefinition = Cast<UPA_CharacterDefinition>(ListItemObject);
	if (CharacterDefinition)
	{
		CharacterIcon->GetDynamicMaterial()->SetTextureParameterValue(IconTextureMatParamName, CharacterDefinition->LoadIcon());
		CharacterNameText->SetText(FText::FromString(CharacterDefinition->GetCharacterDisplayName()));
	}

	SetSelected(false);
}

void UCharacterEntryWidget::SetSelected(bool InbIsSelected)
{
	bIsSelected = InbIsSelected;

	CharacterIcon->GetDynamicMaterial()->SetScalarParameterValue(SaturationMatParamName, bIsSelected ? 0 : 1.f);
}