// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/DamagePopupInstance.h"
#include "Components/WidgetComponent.h"


// Sets default values
ADamagePopupInstance::ADamagePopupInstance()
{
	PrimaryActorTick.bCanEverTick = false;

	DamageTextComponent = CreateDefaultSubobject<UWidgetComponent>("Damage Text Component");
	SetRootComponent(DamageTextComponent);
	DamageTextComponent->SetWidgetSpace(EWidgetSpace::Screen);
}

void ADamagePopupInstance::Show(TSubclassOf<UDamagePopupWidget> WidgetClass, float RawMagnitude, bool bIsHealCue, bool bSecondaryFlag, EDamagePopupDirection Direction, bool bIsItemBonus)
{
	if ( !WidgetClass )
	{
		Destroy();
		return;
	}

	UDamagePopupWidget* Widget = CreateWidget<UDamagePopupWidget>(GetWorld(), WidgetClass);
	if ( !Widget )
	{
		Destroy();
		return;
	}

	Widget->SetDamageValue(RawMagnitude);

	if ( bIsHealCue )
	{
		Widget->SetIsHeal(bSecondaryFlag);
	}
	else
	{
		Widget->SetDamageType(bSecondaryFlag);
	}

	Widget->SetDamageDirection(Direction, bIsItemBonus);
	DamageTextComponent->SetWidget(Widget);
	CachedWidget = Widget;

	CachedWidget->OnFadeOutFinished.AddUObject(this, &ADamagePopupInstance::OnWidgetFadeOutFinished);
	CachedWidget->PlayFadeOut();
}

void ADamagePopupInstance::OnWidgetFadeOutFinished()
{
	Destroy();
}