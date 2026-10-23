// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Items/GA_Item_HolyNecklace.h"
#include "AbilitySystemComponent.h"
#include "GAS/CAbilitySystemStatics.h"

void UGA_Item_HolyNecklace::ApplyToTarget(UAbilitySystemComponent* OwnerASC, UAbilitySystemComponent* TargetASC, AActor* TargetActor)
{
	UCAbilitySystemStatics::ApplyTimedShield(OwnerASC, TargetASC, ShieldGrantEffect, ShieldBuffIconEffect, GetAbilityLevel(), ShieldDuration);

	if ( SpeedBuffEffect )
	{
		FGameplayEffectSpecHandle SpeedSpec = MakeOutgoingGameplayEffectSpec(SpeedBuffEffect, GetAbilityLevel());
		if ( SpeedSpec.IsValid() )
		{
			OwnerASC->ApplyGameplayEffectSpecToTarget(*SpeedSpec.Data.Get(), TargetASC);
		}
	}
}