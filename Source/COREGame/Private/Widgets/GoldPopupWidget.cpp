// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/GoldPopupWidget.h"
#include "Components/TextBlock.h"

void UGoldPopupWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if ( PopupAnim )
	{
		FWidgetAnimationDynamicEvent FinishedEvent;
		FinishedEvent.BindDynamic(this, &UGoldPopupWidget::OnPopupAnimFinished);
		BindToAnimationFinished(PopupAnim, FinishedEvent);
	}
}

void UGoldPopupWidget::SetGoldValue(float GoldGained)
{
	AccumulatedGold = GoldGained;
	UpdateGoldText();
}

void UGoldPopupWidget::AddGoldValue(float AdditionalGold)
{
	AccumulatedGold += AdditionalGold;
	UpdateGoldText();
}

void UGoldPopupWidget::PlayPopup()
{
	if ( PopupAnim )
	{
		if ( IsAnimationPlaying(PopupAnim) ) 
		{
			bSuppressFinishedCallback = true;
			StopAnimation(PopupAnim);
		}

		PlayAnimationForward(PopupAnim);
	}
	else
	{
		OnPopupFinished.Broadcast();
	}
}

void UGoldPopupWidget::UpdateGoldText()
{
	if ( !GoldText )
		return;

	bool bIsSpend = AccumulatedGold < 0.f;
	int32 DisplayValue = FMath::RoundToInt(FMath::Abs(AccumulatedGold));

	GoldText->SetText(FText::FromString(FString::Printf(TEXT("%s%d"), bIsSpend ? TEXT("-") : TEXT("+"), DisplayValue)));
	GoldText->SetColorAndOpacity(FSlateColor(bIsSpend ? SpendColor : GainColor));
}

void UGoldPopupWidget::OnPopupAnimFinished()
{
	if ( bSuppressFinishedCallback )
	{
		bSuppressFinishedCallback = false;
		return;
	}

	OnPopupFinished.Broadcast();
	RemoveFromParent();
}