// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Widgets/AbilityGauge.h"
#include "UltimateAbilityGauge.generated.h"

/**
 * 
 */
UCLASS()
class UUltimateAbilityGauge : public UAbilityGauge
{
	GENERATED_BODY()
	
public:
	virtual void NativeConstruct() override;
	virtual void ConfigureWithWidgetData(const FAbilityWidgetData* WidgetData) override;

protected:
	void UpgradePointUpdated(const FOnAttributeChangeData& Data) override;

private:	
	void LevelUpdated(const FOnAttributeChangeData& Data);
	void UpdateUltUpgradeAvailable();

	UAbilitySystemComponent* OwnerASC;
};
