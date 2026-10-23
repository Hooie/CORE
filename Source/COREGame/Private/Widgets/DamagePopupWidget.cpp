// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/DamagePopupWidget.h"
#include "Components/TextBlock.h"

void UDamagePopupWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if ( FadeOutAnimation )
	{
		FWidgetAnimationDynamicEvent FinishedEvent;
		FinishedEvent.BindDynamic(this, &UDamagePopupWidget::HandleFadeOutAnimationFinished);
		BindToAnimationFinished(FadeOutAnimation, FinishedEvent);
	}
}

void UDamagePopupWidget::SetDamageValue(float DamageValue)
{
	if ( DamageText )
	{
		DamageText->SetText(FText::AsNumber(FMath::RoundToInt(DamageValue)));
	}
}

void UDamagePopupWidget::SetDamageType(bool bIsMagical)
{
	if ( DamageText )
	{
		DamageText->SetColorAndOpacity(FSlateColor(bIsMagical ? MagicalDamageColor : PhysicalDamageColor));
	}
}

void UDamagePopupWidget::PlayFadeOut()
{
	if ( FadeOutAnimation )
	{
		PlayAnimation(FadeOutAnimation);
	}
	else
	{
		HandleFadeOutAnimationFinished();
	}
}

void UDamagePopupWidget::SetDamageDirection(EDamagePopupDirection Direction, bool bIsItemBonus)
{
	if ( !DamageText )
		return;

	FSlateFontInfo Font = DamageText->GetFont();

	switch ( Direction )
	{
	case EDamagePopupDirection::Incoming:
		Font.Size = IncomingFontSize;
		Font.TypefaceFontName = FName("Bold");
		break;

	case EDamagePopupDirection::OutGoing:
		Font.Size = OutgoingFontSize;
		Font.TypefaceFontName = FName("Bold Italic");
		break;

	case EDamagePopupDirection::Neutral:
		Font.Size = NeutralFontSize;
		Font.TypefaceFontName = FName("Italic");
		break;
	}

	if ( bIsItemBonus )
	{
		Font.Size = FMath::Max(1, FMath::RoundToInt(Font.Size * ItemBonusFontScale));
	}

	DamageText->SetFont(Font);
}

void UDamagePopupWidget::SetIsHeal(bool bIsLifeSteal)
{
	if ( DamageText )
	{
		DamageText->SetColorAndOpacity(FSlateColor(bIsLifeSteal ? LifeStealColor : HealColor));
	}
}

void UDamagePopupWidget::HandleFadeOutAnimationFinished()
{
	OnFadeOutFinished.Broadcast();
}
