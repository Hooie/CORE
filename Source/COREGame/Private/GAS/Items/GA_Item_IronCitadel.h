// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GAS/CGameplayAbility.h"
#include "GA_Item_IronCitadel.generated.h"

/**
 * 
 */
UCLASS()
class UGA_Item_IronCitadel : public UCGameplayAbility
{
	GENERATED_BODY()
	
public:
	UGA_Item_IronCitadel();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

private:
	UPROPERTY(EditDefaultsOnly, Category = "Iron Citadel")
	TSubclassOf<UGameplayEffect> ShieldGrantEffect;
	
	UPROPERTY(EditDefaultsOnly, Category = "Iron Citadel")
	TSubclassOf<UGameplayEffect> ShieldBuffIconEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Iron Citadel")
	float ShieldDuration = 2.f;
};
