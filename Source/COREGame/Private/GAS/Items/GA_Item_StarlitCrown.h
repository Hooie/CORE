// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GAS/CGameplayAbility.h"
#include "GA_Item_StarlitCrown.generated.h"

/**
 * 
 */
UCLASS()
class UGA_Item_StarlitCrown : public UCGameplayAbility
{
	GENERATED_BODY()
	
public:
	UGA_Item_StarlitCrown();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

private:
	UPROPERTY(EditDefaultsOnly, Category = "StarlitCrown")
	float ShieldRadius = 900.f;

	UPROPERTY(EditDefaultsOnly, Category = "StarlitCrown")
	float ShieldDuration = 4.f;

	UPROPERTY(EditDefaultsOnly, Category = "StarlitCrown")
	bool bIncludeSelf = true;

	UPROPERTY(EditDefaultsOnly, Category = "StarlitCrown")
	TSubclassOf<UGameplayEffect> ShieldGrantEffect;

	UPROPERTY(EditDefaultsOnly, Category = "StarlitCrown")
	TSubclassOf<UGameplayEffect> ShieldBuffIconEffect;

	static FGameplayTag GetStarlitCrownCueTag();
};
