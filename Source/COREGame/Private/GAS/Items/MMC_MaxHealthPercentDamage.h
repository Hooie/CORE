// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "MMC_MaxHealthPercentDamage.generated.h"

/**
 * 
 */
UCLASS()
class UMMC_MaxHealthPercentDamage : public UGameplayModMagnitudeCalculation
{
	GENERATED_BODY()
	
public:
	UMMC_MaxHealthPercentDamage();

	virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;

private:
	FGameplayEffectAttributeCaptureDefinition MaxHealthCaptureDef;
	FGameplayEffectAttributeCaptureDefinition ArmorCaptureDef;
};
