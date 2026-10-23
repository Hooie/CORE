// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/CGameplayAbilityTypes.h"

FGenericDamageEffectDef::FGenericDamageEffectDef()
	:DamageEffect{ nullptr },
	PushVelocity{ 0.f },
	ScalingRatio{ 1.f }
{
}

FHeroBaseStats::FHeroBaseStats()
	:Class{ nullptr },
	Strength{ 0 },
	StrengthGrowthRate{ 0 },
	Intelligence{ 0 },
	IntelligenceGrowthRate{ 0 },
	BaseMaxHealth{ 0 },
	MaxHealthGrowthRate{ 0 },        
	BaseMaxMana{ 0 },
	MaxManaGrowthRate{ 0 },          
	BaseHealthRegen{ 0 },
	HealthRegenGrowthRate{ 0 },
	BaseManaRegen{ 0 },
	ManaRegenGrowthRate{ 0 },
	BaseArmor{ 0 },
	ArmorGrowthRate{ 0 },            
	BaseMagicResistance{ 0 },        
	MagicResistanceGrowthRate{ 0 },
	BaseMoveSpeed{ 0 },
	BaseMoveAcceleration{ 2048.f }
{
}