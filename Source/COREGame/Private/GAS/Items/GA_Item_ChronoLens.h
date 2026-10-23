// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GAS/CGameplayAbility.h"
#include "GA_Item_ChronoLens.generated.h"

/**
 * 
 */
UCLASS()
class UGA_Item_ChronoLens : public UCGameplayAbility
{
	GENERATED_BODY()
	
public:
	UGA_Item_ChronoLens();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

private:
	UPROPERTY(EditDefaultsOnly, Category = "ChronoLens")
	float InvincibleDuration = 2.5f;

	FTimerHandle DurationTimerHandle;
};
