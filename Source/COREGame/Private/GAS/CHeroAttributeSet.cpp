// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/CHeroAttributeSet.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffectExtension.h"

void UCHeroAttributeSet::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	// 다른 클라이언트도 필요
	DOREPLIFETIME_CONDITION_NOTIFY(UCHeroAttributeSet, Intelligence, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UCHeroAttributeSet, Strength, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UCHeroAttributeSet, Level, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UCHeroAttributeSet, MaxLevel, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UCHeroAttributeSet, LifeStealRatio, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UCHeroAttributeSet, HealthRegen, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UCHeroAttributeSet, ManaRegen, COND_None, REPNOTIFY_Always);

	// 본인 HUD에서만 사용 (미니언/포탑은 아무에게도 복제되지 않음)
	DOREPLIFETIME_CONDITION_NOTIFY(UCHeroAttributeSet, Gold, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UCHeroAttributeSet, Experience, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UCHeroAttributeSet, PrevLevelExperience, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UCHeroAttributeSet, NextLevelExperience, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UCHeroAttributeSet, MaxLevelExperience, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UCHeroAttributeSet, UpgradePoint, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UCHeroAttributeSet, IntelligenceGrowthRate, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UCHeroAttributeSet, StrengthGrowthRate, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UCHeroAttributeSet, MaxHealthGrowthRate, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UCHeroAttributeSet, MaxManaGrowthRate, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UCHeroAttributeSet, ArmorGrowthRate, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UCHeroAttributeSet, MagicResistanceGrowthRate, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UCHeroAttributeSet, HealthRegenGrowthRate, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UCHeroAttributeSet, ManaRegenGrowthRate, COND_OwnerOnly, REPNOTIFY_Always);
}

void UCHeroAttributeSet::OnRep_Intelligence(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UCHeroAttributeSet, Intelligence, OldValue);
}

void UCHeroAttributeSet::OnRep_Strength(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UCHeroAttributeSet, Strength, OldValue);
}

void UCHeroAttributeSet::OnRep_IntelligenceGrowthRate(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UCHeroAttributeSet, IntelligenceGrowthRate, OldValue);
}

void UCHeroAttributeSet::OnRep_StrengthGrowthRate(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UCHeroAttributeSet, StrengthGrowthRate, OldValue);
}

void UCHeroAttributeSet::OnRep_MaxHealthGrowthRate(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UCHeroAttributeSet, MaxHealthGrowthRate, OldValue);
}

void UCHeroAttributeSet::OnRep_MaxManaGrowthRate(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UCHeroAttributeSet, MaxManaGrowthRate, OldValue);
}

void UCHeroAttributeSet::OnRep_ArmorGrowthRate(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UCHeroAttributeSet, ArmorGrowthRate, OldValue);
}

void UCHeroAttributeSet::OnRep_MagicResistanceGrowthRate(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UCHeroAttributeSet, MagicResistanceGrowthRate, OldValue);
}

void UCHeroAttributeSet::OnRep_Experience(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UCHeroAttributeSet, Experience, OldValue);
}

void UCHeroAttributeSet::OnRep_PrevLevelExperience(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UCHeroAttributeSet, PrevLevelExperience, OldValue);
}

void UCHeroAttributeSet::OnRep_NextLevelExperience(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UCHeroAttributeSet, NextLevelExperience, OldValue);
}

void UCHeroAttributeSet::OnRep_Level(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UCHeroAttributeSet, Level, OldValue);
}

void UCHeroAttributeSet::OnRep_UpgradePoint(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UCHeroAttributeSet, UpgradePoint, OldValue);
}

void UCHeroAttributeSet::OnRep_MaxLevel(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UCHeroAttributeSet, MaxLevel, OldValue);
}

void UCHeroAttributeSet::OnRep_MaxLevelExperience(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UCHeroAttributeSet, MaxLevelExperience, OldValue);
}

void UCHeroAttributeSet::OnRep_Gold(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UCHeroAttributeSet, Gold, OldValue);
}

void UCHeroAttributeSet::OnRep_LifeStealRatio(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UCHeroAttributeSet, LifeStealRatio, OldValue);
}

void UCHeroAttributeSet::OnRep_HealthRegen(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UCHeroAttributeSet, HealthRegen, OldValue);
}

void UCHeroAttributeSet::OnRep_HealthRegenGrowthRate(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UCHeroAttributeSet, HealthRegenGrowthRate, OldValue);
}

void UCHeroAttributeSet::OnRep_ManaRegen(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UCHeroAttributeSet, ManaRegen, OldValue);
}

void UCHeroAttributeSet::OnRep_ManaRegenGrowthRate(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UCHeroAttributeSet, ManaRegenGrowthRate, OldValue);
}
