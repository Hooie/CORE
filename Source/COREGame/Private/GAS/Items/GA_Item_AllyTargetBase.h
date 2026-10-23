// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GAS/CGameplayAbility.h"
#include "GA_Item_AllyTargetBase.generated.h"

/**
 * 
 */
UCLASS(Abstract)
class UGA_Item_AllyTargetBase : public UCGameplayAbility
{
	GENERATED_BODY()
	
public:
	UGA_Item_AllyTargetBase();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	virtual void ApplyToTarget(UAbilitySystemComponent* OwnerASC, UAbilitySystemComponent* TargetASC, AActor* TargetActor) {}

	UPROPERTY(EditDefaultsOnly, Category = "Targeting")
	TSubclassOf<class ATA_AllySelector> SelectorClass;

	UPROPERTY(EditDefaultsOnly, Category = "Targeting")
	float TargetRange = 1000.f;

	UPROPERTY(EditDefaultsOnly, Category = "Targeting")
	bool bAllowSelfTarget = true;

	UPROPERTY(EditDefaultsOnly, Category = "Targeting")
	float ServerRangeTolerance = 200.f;

	UPROPERTY(EditDefaultsOnly, Category = "Targeting", meta = (Categories = "GameplayCue"))
	FGameplayTag CasterCueTag;
	
	UPROPERTY(EditDefaultsOnly, Category = "Targeting", meta = (Categories = "GameplayCue"))
	FGameplayTag TargetCueTag;

private:
	UFUNCTION()
	void OnTargetConfirmed(const FGameplayAbilityTargetDataHandle& TargetDataHandle);

	UFUNCTION()
	void OnTargetCancelled(const FGameplayAbilityTargetDataHandle& TargetDataHandle);

	bool IsValidTarget(const AActor* Target) const;
};
