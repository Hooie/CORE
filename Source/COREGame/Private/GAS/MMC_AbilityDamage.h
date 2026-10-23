// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "MMC_AbilityDamage.generated.h"

/**
 * 
 */
UCLASS()
class UMMC_AbilityDamage : public UGameplayModMagnitudeCalculation
{
	GENERATED_BODY()
	
public:
	UMMC_AbilityDamage();

	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;

	static bool PreviewRawDamage(TSubclassOf<class UGameplayEffect> DamageEffect, const class UAbilitySystemComponent* SourceASC,
		float BaseDamage, float LegacyRatio, float StrengthRatio, float IntelligenceRatio,
		float& OutRawDamage, bool& bOutIsMagical);

	static float ApplyDefense(float RawDamage, float DefenseValue)
	{
		return RawDamage * (1.f - (DefenseValue / (DefenseValue + 100.f)));
	}

private:
	UPROPERTY(EditDefaultsOnly, Category = "Scaling")
	FGameplayAttribute ScalingAttribute;

	FGameplayEffectAttributeCaptureDefinition ArmorCaptureDef;
	FGameplayEffectAttributeCaptureDefinition MagicResistanceCaptureDef;
};
