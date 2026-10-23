// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GAS/CGameplayAbility.h"
#include "GA_Item_PheonixAegis.generated.h"

/**
 * 
 */
UCLASS()
class UGA_Item_PheonixAegis : public UCGameplayAbility
{
	GENERATED_BODY()
	
public:
	UGA_Item_PheonixAegis();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	
private:
	UPROPERTY(EditDefaultsOnly, Category = "PheonixAegis")
	TSubclassOf<UGameplayEffect> RegenBuffEffect;

	UPROPERTY(EditDefaultsOnly, Category = "PheonixAegis")
	float AuraRadius = 500.f;

	UPROPERTY(EditDefaultsOnly, Category = "PheonixAegis")
	float BaseDamagePerTick = 25.f;

	UPROPERTY(EditDefaultsOnly, Category = "PheonixAegis")
	float MaxHealthRatioPerTick = 0.01f;

	UPROPERTY(EditDefaultsOnly, Category = "PheonixAegis")
	float TickInterval = 1.f;

	UPROPERTY(EditDefaultsOnly, Category = "PheonixAegis")
	int32 TickCount = 5;

	UPROPERTY(EditDefaultsOnly, Category = "PheonixAegis", meta = (Categories = "GameplayCue"))
	FGameplayTag AuraCueTag;

	FTimerHandle AuraTickTimerHandle;
	int32 RemainingTicks = 0;

	void TickAura();
};
