// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "CHeroAttributeSet.generated.h"

#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
 	GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
 	GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
 	GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
 	GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)
/**
 * 
 */
UCLASS()
class UCHeroAttributeSet : public UAttributeSet
{
	GENERATED_BODY()
public:
    ATTRIBUTE_ACCESSORS(UCHeroAttributeSet, Intelligence)
    ATTRIBUTE_ACCESSORS(UCHeroAttributeSet, Strength)
    ATTRIBUTE_ACCESSORS(UCHeroAttributeSet, IntelligenceGrowthRate)
    ATTRIBUTE_ACCESSORS(UCHeroAttributeSet, StrengthGrowthRate)
	ATTRIBUTE_ACCESSORS(UCHeroAttributeSet, MaxHealthGrowthRate)        
	ATTRIBUTE_ACCESSORS(UCHeroAttributeSet, MaxManaGrowthRate)          
	ATTRIBUTE_ACCESSORS(UCHeroAttributeSet, ArmorGrowthRate)            
	ATTRIBUTE_ACCESSORS(UCHeroAttributeSet, MagicResistanceGrowthRate)
    ATTRIBUTE_ACCESSORS(UCHeroAttributeSet, Experience)
    ATTRIBUTE_ACCESSORS(UCHeroAttributeSet, PrevLevelExperience)
    ATTRIBUTE_ACCESSORS(UCHeroAttributeSet, NextLevelExperience)
    ATTRIBUTE_ACCESSORS(UCHeroAttributeSet, Level)
    ATTRIBUTE_ACCESSORS(UCHeroAttributeSet, UpgradePoint)
    ATTRIBUTE_ACCESSORS(UCHeroAttributeSet, MaxLevel)
    ATTRIBUTE_ACCESSORS(UCHeroAttributeSet, MaxLevelExperience)
    ATTRIBUTE_ACCESSORS(UCHeroAttributeSet, Gold)
    ATTRIBUTE_ACCESSORS(UCHeroAttributeSet, LifeStealRatio)
	ATTRIBUTE_ACCESSORS(UCHeroAttributeSet, HealthRegen)
	ATTRIBUTE_ACCESSORS(UCHeroAttributeSet, HealthRegenGrowthRate)
	ATTRIBUTE_ACCESSORS(UCHeroAttributeSet, ManaRegen)
	ATTRIBUTE_ACCESSORS(UCHeroAttributeSet, ManaRegenGrowthRate)

	virtual void GetLifetimeReplicatedProps( TArray< class FLifetimeProperty > & OutLifetimeProps ) const override;

private:
	UPROPERTY(ReplicatedUsing = OnRep_Intelligence)
	FGameplayAttributeData Intelligence;

	UPROPERTY(ReplicatedUsing = OnRep_Strength)
	FGameplayAttributeData Strength;
	
	UPROPERTY(ReplicatedUsing = OnRep_Experience)
	FGameplayAttributeData Experience;

	UPROPERTY(ReplicatedUsing = OnRep_IntelligenceGrowthRate)
	FGameplayAttributeData IntelligenceGrowthRate;

	UPROPERTY(ReplicatedUsing = OnRep_StrengthGrowthRate)
	FGameplayAttributeData StrengthGrowthRate;

	UPROPERTY(ReplicatedUsing = OnRep_MaxHealthGrowthRate)
	FGameplayAttributeData MaxHealthGrowthRate;

	UPROPERTY(ReplicatedUsing = OnRep_MaxManaGrowthRate)
	FGameplayAttributeData MaxManaGrowthRate;

	UPROPERTY(ReplicatedUsing = OnRep_ArmorGrowthRate)
	FGameplayAttributeData ArmorGrowthRate;

	UPROPERTY(ReplicatedUsing = OnRep_MagicResistanceGrowthRate)
	FGameplayAttributeData MagicResistanceGrowthRate;

	UPROPERTY(ReplicatedUsing = OnRep_PrevLevelExperience)
	FGameplayAttributeData PrevLevelExperience;

	UPROPERTY(ReplicatedUsing = OnRep_NextLevelExperience)
	FGameplayAttributeData NextLevelExperience;

	UPROPERTY(ReplicatedUsing = OnRep_Level)
	FGameplayAttributeData Level;

	UPROPERTY(ReplicatedUsing = OnRep_UpgradePoint)
	FGameplayAttributeData UpgradePoint;
	
	UPROPERTY(ReplicatedUsing = OnRep_MaxLevel)
	FGameplayAttributeData MaxLevel;

	UPROPERTY(ReplicatedUsing = OnRep_MaxLevel)
	FGameplayAttributeData MaxLevelExperience;

	UPROPERTY(ReplicatedUsing = OnRep_Gold)
	FGameplayAttributeData Gold;

	UPROPERTY(ReplicatedUsing = OnRep_LifeStealRatio)
	FGameplayAttributeData LifeStealRatio;

	UPROPERTY(ReplicatedUsing = OnRep_HealthRegen)
	FGameplayAttributeData HealthRegen;

	UPROPERTY(ReplicatedUsing = OnRep_HealthRegenGrowthRate)
	FGameplayAttributeData HealthRegenGrowthRate;

	UPROPERTY(ReplicatedUsing = OnRep_ManaRegen)
	FGameplayAttributeData ManaRegen;

	UPROPERTY(ReplicatedUsing = OnRep_ManaRegenGrowthRate)
	FGameplayAttributeData ManaRegenGrowthRate;

	UFUNCTION()
	void OnRep_Intelligence(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_Strength(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_IntelligenceGrowthRate(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_StrengthGrowthRate(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_MaxHealthGrowthRate(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_MaxManaGrowthRate(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_ArmorGrowthRate(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_MagicResistanceGrowthRate(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_Experience(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_PrevLevelExperience(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_NextLevelExperience(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_Level(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_UpgradePoint(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_MaxLevel(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_MaxLevelExperience(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_Gold(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_LifeStealRatio(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_HealthRegen(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_HealthRegenGrowthRate(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_ManaRegen(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	void OnRep_ManaRegenGrowthRate(const FGameplayAttributeData& OldValue);
};
