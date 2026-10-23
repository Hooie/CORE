// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GAS/Items/GA_Item_AllyTargetBase.h"
#include "GA_Item_HolyNecklace.generated.h"

/**
 * 
 */
UCLASS()
class UGA_Item_HolyNecklace : public UGA_Item_AllyTargetBase
{
	GENERATED_BODY()
	
protected:
	virtual void ApplyToTarget(UAbilitySystemComponent* OwnerASC, UAbilitySystemComponent* TargetASC, AActor* TargetActor) override;

private:
	UPROPERTY(EditDefaultsOnly, Category = "HolyNecklace")
	float ShieldDuration = 5.f;

	UPROPERTY(EditDefaultsOnly, Category = "HolyNecklace")
	TSubclassOf<UGameplayEffect> ShieldGrantEffect;

	UPROPERTY(EditDefaultsOnly, Category = "HolyNecklace")
	TSubclassOf<UGameplayEffect> ShieldBuffIconEffect;

	UPROPERTY(EditDefaultsOnly, Category = "HolyNecklace")
	TSubclassOf<UGameplayEffect> SpeedBuffEffect;
};
