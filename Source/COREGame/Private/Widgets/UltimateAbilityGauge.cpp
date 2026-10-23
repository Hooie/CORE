// Fill out your copyright notice in the Description page of Project Settings.


#include "Widgets/UltimateAbilityGauge.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GAS/CAbilitySystemStatics.h"
#include "GAS/CHeroAttributeSet.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

void UUltimateAbilityGauge::NativeConstruct()
{
	OwnerASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwningPlayerPawn());
	
	Super::NativeConstruct();

	if ( OwnerASC )
	{
		OwnerASC->GetGameplayAttributeValueChangeDelegate(UCHeroAttributeSet::GetLevelAttribute()).AddUObject(this, &UUltimateAbilityGauge::LevelUpdated);
		OwnerASC->GetGameplayAttributeValueChangeDelegate(UCHeroAttributeSet::GetUpgradePointAttribute()).RemoveAll(this);
		OwnerASC->GetGameplayAttributeValueChangeDelegate(UCHeroAttributeSet::GetUpgradePointAttribute()).AddUObject(this, &UUltimateAbilityGauge::UpgradePointUpdated);
	}
}

void UUltimateAbilityGauge::ConfigureWithWidgetData(const FAbilityWidgetData* WidgetData)
{
	Super::ConfigureWithWidgetData(WidgetData);

	UpdateUltUpgradeAvailable();
}

void UUltimateAbilityGauge::UpgradePointUpdated(const FOnAttributeChangeData& Data)
{
	UpdateUltUpgradeAvailable();
}

void UUltimateAbilityGauge::LevelUpdated(const FOnAttributeChangeData& Data)
{
	UpdateUltUpgradeAvailable();
}

void UUltimateAbilityGauge::UpdateUltUpgradeAvailable()
{
	if ( !OwnerASC )
		return;

	const FGameplayAbilitySpec* AbilitySpec = GetAbilitySpec();
	if ( !AbilitySpec )
		return;

	if ( UCAbilitySystemStatics::IsAbilityAtMaxLevel(*AbilitySpec) )
	{
		Icon->GetDynamicMaterial()->SetScalarParameterValue(UpgradePointAvailableParamName, 0);
		return;
	}

	bool bFound = false;
	float UpgradePoint = OwnerASC->GetGameplayAttributeValue(UCHeroAttributeSet::GetUpgradePointAttribute(), bFound);
	if ( !bFound || UpgradePoint <= 0 )
	{
		Icon->GetDynamicMaterial()->SetScalarParameterValue(UpgradePointAvailableParamName, 0);
		return;
	}

	float CurrentLevel = OwnerASC->GetGameplayAttributeValue(UCHeroAttributeSet::GetLevelAttribute(), bFound);
	int NextUltLevel = AbilitySpec->Level;
	bool bCanUpgrade = NextUltLevel < UCAbilitySystemStatics::UltimateMaxLevel && CurrentLevel >= UCAbilitySystemStatics::UltRequiredLevel[ NextUltLevel ];

	Icon->GetDynamicMaterial()->SetScalarParameterValue(UpgradePointAvailableParamName, bCanUpgrade ? 1 : 0);
}
