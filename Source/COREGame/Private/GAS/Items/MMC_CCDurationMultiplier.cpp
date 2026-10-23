// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Items/MMC_CCDurationMultiplier.h"
#include "GAS/CAbilitySystemStatics.h"
#include "GAS/CGameplayEffectSettings.h"

float UMMC_CCDurationMultiplier::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	if ( TargetTags && TargetTags->HasTag(UCAbilitySystemStatics::GetStepOfHermesEquippedTag()) )
	{
		const UCGameplayEffectSettings* Settings = GetDefault<UCGameplayEffectSettings>();

		return Settings ? Settings->HermesCCDurationMultiplier : 0.75f;
	}

	return 1.0f;
}
