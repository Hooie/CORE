// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Items/MMC_MaxHealthPercentDamage.h"
#include "GAS/CAttributeSet.h"
#include "GAS/CAbilitySystemStatics.h"

UMMC_MaxHealthPercentDamage::UMMC_MaxHealthPercentDamage()
{
	MaxHealthCaptureDef.AttributeToCapture = UCAttributeSet::GetMaxHealthAttribute();
	MaxHealthCaptureDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Target;
	RelevantAttributesToCapture.Add(MaxHealthCaptureDef);

	ArmorCaptureDef.AttributeToCapture = UCAttributeSet::GetArmorAttribute();
	ArmorCaptureDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Target;
	RelevantAttributesToCapture.Add(ArmorCaptureDef);
}

float UMMC_MaxHealthPercentDamage::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	FAggregatorEvaluateParameters EvalParams;
	EvalParams.SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	EvalParams.TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	float TargetMaxHealth = 0.f;
	GetCapturedAttributeMagnitude(MaxHealthCaptureDef, Spec, EvalParams, TargetMaxHealth);
	
	float TargetArmor = 0.f;
	GetCapturedAttributeMagnitude(ArmorCaptureDef, Spec, EvalParams, TargetArmor);

	float Percent = Spec.GetSetByCallerMagnitude(UCAbilitySystemStatics::GetMaxHealthPercentDataTag(), false, 0.f);

	float RawDamage = TargetMaxHealth * Percent;
	float FinalDamage = RawDamage * (1.f - (TargetArmor / (TargetArmor + 100.f)));
	
	return -FinalDamage;
}
