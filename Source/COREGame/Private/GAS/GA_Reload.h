// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GAS/CGameplayAbility.h"
#include "GA_Reload.generated.h"

/**
 * 
 */
UCLASS()
class UGA_Reload : public UCGameplayAbility
{
	GENERATED_BODY()
	
public:
	UGA_Reload();

	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const override;
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	
	static FGameplayTag GetReloadRefillTag();

private:
	UPROPERTY(EditDefaultsOnly, Category = "Anim")
	UAnimMontage* ReloadMontage;

	UFUNCTION()
	void OnReloadMontageFinished();

	UFUNCTION()
	void OnReloadRefillEvent(FGameplayEventData Payload);
};
