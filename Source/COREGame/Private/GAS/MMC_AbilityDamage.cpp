// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/MMC_AbilityDamage.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "GAS/CAbilitySystemStatics.h"
#include "GAS/CAttributeSet.h"
#include "GAS/CGameplayEffectSettings.h"
#include "GAS/CHeroAttributeSet.h"
#include "GAS/Items/CItemStatics.h"

UMMC_AbilityDamage::UMMC_AbilityDamage()
{
	ArmorCaptureDef.AttributeToCapture = UCAttributeSet::GetArmorAttribute();
	ArmorCaptureDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Target;
	RelevantAttributesToCapture.Add(ArmorCaptureDef);

	MagicResistanceCaptureDef.AttributeToCapture = UCAttributeSet::GetMagicResistanceAttribute();
	MagicResistanceCaptureDef.AttributeSource = EGameplayEffectAttributeCaptureSource::Target;
	RelevantAttributesToCapture.Add(MagicResistanceCaptureDef);
}

float UMMC_AbilityDamage::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
	FAggregatorEvaluateParameters EvalParams;
	EvalParams.SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	EvalParams.TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	FGameplayTagContainer EffectAssetTags;
	Spec.GetAllAssetTags(EffectAssetTags);

	const bool bIsMagical = EffectAssetTags.HasTag(UCAbilitySystemStatics::GetMagicalDamageTypeTag());
	const bool bIsBasicAttack = EffectAssetTags.HasTag(UCAbilitySystemStatics::GetBasicAttackTypeTag());
	const bool bIsItemDamage = EffectAssetTags.HasTag(UCAbilitySystemStatics::GetItemBonusDamageTag());

	float DefenseValue = 0.f;
	GetCapturedAttributeMagnitude(bIsMagical ? MagicResistanceCaptureDef : ArmorCaptureDef, Spec, EvalParams, DefenseValue);

	DefenseValue = FMath::Max(0.f, DefenseValue * UCItemStatics::GetDefensePenetrationMultiplier(EvalParams.SourceTags, bIsMagical));

	UAbilitySystemComponent* SourceASC = Spec.GetContext().GetInstigatorAbilitySystemComponent();

	float LegacyScalingValue = 0.f;
	if ( ScalingAttribute.IsValid() && SourceASC )
	{
		bool bFound = false;
		LegacyScalingValue = SourceASC->GetGameplayAttributeValue(ScalingAttribute, bFound);
	}
	float LegacyRatio = Spec.GetSetByCallerMagnitude(UCAbilitySystemStatics::GetAbilityScalingRatioTag(), false, 0.f);

	float StrengthValue = 0.f;
	float IntelligenceValue = 0.f;
	if ( SourceASC )
	{
		bool bFound = false;
		StrengthValue = SourceASC->GetGameplayAttributeValue(UCHeroAttributeSet::GetStrengthAttribute(), bFound);
		IntelligenceValue = SourceASC->GetGameplayAttributeValue(UCHeroAttributeSet::GetIntelligenceAttribute(), bFound);
	}
	float StrengthRatio = Spec.GetSetByCallerMagnitude(UCAbilitySystemStatics::GetDamageStrengthRatioTag(), false, 0.f);
	float IntelligenceRatio = Spec.GetSetByCallerMagnitude(UCAbilitySystemStatics::GetDamageIntelligenceRatioTag(), false, 0.f);

	float BaseDamage = Spec.GetSetByCallerMagnitude(UCAbilitySystemStatics::GetAbilityBaseDamageTag(), false, 0.f);
	float RawDamage = BaseDamage 
					+ LegacyScalingValue * LegacyRatio 
					+ StrengthValue * StrengthRatio 
					+ IntelligenceValue * IntelligenceRatio;

	if ( !bIsBasicAttack && !bIsItemDamage && EvalParams.SourceTags && EvalParams.SourceTags->HasTag(UCAbilitySystemStatics::GetArchmageSealEquippedTag()) )
	{
		const UCGameplayEffectSettings* Settings = GetDefault<UCGameplayEffectSettings>();
		RawDamage *= 1.f + (Settings ? Settings->ArchmageSealSkillAmp : 0.f);
	}

	RawDamage *= UCItemStatics::GetDamageMultiplier(EvalParams.SourceTags, EvalParams.TargetTags, bIsBasicAttack);

	return -ApplyDefense(RawDamage, DefenseValue);
}

bool UMMC_AbilityDamage::PreviewRawDamage(TSubclassOf<class UGameplayEffect> DamageEffect, const UAbilitySystemComponent* SourceASC, float BaseDamage, float LegacyRatio, float StrengthRatio, float IntelligenceRatio, float& OutRawDamage, bool& bOutIsMagical)
{
	OutRawDamage = 0.f;
	bOutIsMagical = false;

	if ( !DamageEffect || !SourceASC )
		return false;

	const UGameplayEffect* EffectCDO = DamageEffect->GetDefaultObject<UGameplayEffect>();
	if ( !EffectCDO )
		return false;

	bOutIsMagical = EffectCDO->GetAssetTags().HasTag(UCAbilitySystemStatics::GetMagicalDamageTypeTag());

	FGameplayAttribute LegacyAttribute;
	for ( const FGameplayModifierInfo& Modifier : EffectCDO->Modifiers )
	{
		TSubclassOf<UGameplayModMagnitudeCalculation> CalcClass = Modifier.ModifierMagnitude.GetCustomMagnitudeCalculationClass();
		const UMMC_AbilityDamage* DamageCalc = CalcClass ? Cast<UMMC_AbilityDamage>(CalcClass->GetDefaultObject()) : nullptr;

		if ( DamageCalc )
		{
			LegacyAttribute = DamageCalc->ScalingAttribute;
			break;
		}
	}

	bool bFound = false;
	const float LegacyValue = LegacyAttribute.IsValid() ? SourceASC->GetGameplayAttributeValue(LegacyAttribute, bFound) : 0.f;
	const float StrengthValue = SourceASC->GetGameplayAttributeValue(UCHeroAttributeSet::GetStrengthAttribute(), bFound);
	const float IntelligenceValue = SourceASC->GetGameplayAttributeValue(UCHeroAttributeSet::GetIntelligenceAttribute(), bFound);

	OutRawDamage = BaseDamage
		+ LegacyRatio * LegacyValue
		+ StrengthRatio * StrengthValue
		+ IntelligenceRatio * IntelligenceValue;

	return true;
}
