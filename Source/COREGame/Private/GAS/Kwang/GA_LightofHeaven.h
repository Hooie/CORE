// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GAS/CGameplayAbility.h"
#include "GA_LightofHeaven.generated.h"

/**
 *
 */
UCLASS()
class UGA_LightofHeaven : public UCGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_LightofHeaven();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	static FGameplayTag GetActivateEventTag();
	static FGameplayTag GetNoWeaponTag();

private:
	UPROPERTY(EditDefaultsOnly, Category = "Effect")
	TSubclassOf<UGameplayEffect> BuffEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Effect")
	TSubclassOf<UGameplayEffect> DamageEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Targeting")
	float DamageRange = 200.f;

	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	TArray<float> BaseDamagePerLevel = { 65.f, 95.f, 125.f, 185.f, 250.f };

	UPROPERTY(EditDefaultsOnly, Category = "Damage")
	float ScalingRatio = 1.1f;

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimMontage* ActivateMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimMontage* UnarmedActivateMontage;

	UFUNCTION()
	void GrantBuffAndDealDamage(FGameplayEventData Data);
};
