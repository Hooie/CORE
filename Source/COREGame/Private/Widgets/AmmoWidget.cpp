// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/AmmoWidget.h"
#include "Character/CMurdockCharacter.h"
#include "Components/TextBlock.h"
#include "Player/AmmoComponent.h"

void UAmmoWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if ( ACMurdockCharacter* MurdockCharacter = Cast<ACMurdockCharacter>(GetOwningPlayerPawn()))
	{
		if ( UAmmoComponent* AmmoComponent = MurdockCharacter->GetComponentByClass<UAmmoComponent>() )
		{
			AmmoComponent->OnAmmoChanged.AddUObject(this, &UAmmoWidget::OnAmmoChanged);
			OnAmmoChanged(AmmoComponent->GetCurrentAmmo(), AmmoComponent->GetMaxAmmo());
		}
	}
}

void UAmmoWidget::NativeDestruct()
{
	if ( ACMurdockCharacter* MurdockCharacter = Cast<ACMurdockCharacter>(GetOwningPlayerPawn()) )
	{
		if ( UAmmoComponent* AmmoComponent = MurdockCharacter->GetComponentByClass<UAmmoComponent>() )
		{
			AmmoComponent->OnAmmoChanged.RemoveAll(this);
		}
	}

	Super::NativeDestruct();
}

void UAmmoWidget::OnAmmoChanged(int32 CurrentAmmo, int32 MaxAmmo)
{
	if ( AmmoText )
	{
		AmmoText->SetText(FText::FromString(FString::Printf(TEXT("%d / %d"), CurrentAmmo, MaxAmmo)));
	}
}
