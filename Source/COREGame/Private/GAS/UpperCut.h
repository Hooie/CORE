// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GAS/CGameplayAbility.h"
#include "GAS/CGameplayAbilityTypes.h"
#include "UpperCut.generated.h"

/**
 * 
 */
UCLASS()
class UUpperCut : public UCGameplayAbility
{
	GENERATED_BODY()

public:
	UUpperCut();
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

private:
	UPROPERTY(EditDefaultsOnly, Category = "Combo")
	TMap<FName, FGenericDamageEffectDef> ComboDamageMap;

	UPROPERTY(EditDefaultsOnly, Category = "Launch")
	TSubclassOf<UGameplayEffect> LaunchDamageEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Launch")
	float UpperCutLaunchSpeed = 1000.f;

	UPROPERTY(EditDefaultsOnly, Category = "Launch")
	float UpperCutComboHoldSpeed = 100.f;

	UPROPERTY(EditDefaultsOnly, Category = "Launch|Damage")
	TArray<float> LaunchBaseDamagePerLevel = { 50.f, 75.f, 100.f, 125.f, 150.f };

	UPROPERTY(EditDefaultsOnly, Category = "Launch|Damage")
	float LaunchScalingRatio = 0.7f;

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimMontage* UpperCutMontage;

	static FGameplayTag GetUpperCutLaunchTag();

	const FGenericDamageEffectDef* GetDamageEffectDefFromCurrentCombo() const;

	UFUNCTION()
	void StartLaunching(FGameplayEventData EventData);

	UFUNCTION()
	void HandleComboChangeEvent(FGameplayEventData EventData);

	UFUNCTION()
	void HandleComboCommitEvent(FGameplayEventData EventData);
	
	UFUNCTION()
	void HandleComboDamageEvent(FGameplayEventData EventData);

	FName NextComboName;
};
