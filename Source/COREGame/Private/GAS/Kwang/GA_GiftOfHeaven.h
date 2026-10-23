// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GAS/CGameplayAbility.h"
#include "GA_GiftOfHeaven.generated.h"

/**
 * 
 */
UCLASS()
class UGA_GiftOfHeaven : public UCGameplayAbility
{
	GENERATED_BODY()
	
public:
	UGA_GiftOfHeaven();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	virtual void OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo,const FGameplayAbilitySpec& Spec) override;

private:
	UPROPERTY(EditDefaultsOnly, Category = "Effect")
	TSubclassOf<UGameplayEffect> LifeStealBuffEffect;
	
	FActiveGameplayEffectHandle LifeStealBuffHandle;

	UFUNCTION()
	void OnAbilitySpecDirtied(const FGameplayAbilitySpec& Spec);
};
