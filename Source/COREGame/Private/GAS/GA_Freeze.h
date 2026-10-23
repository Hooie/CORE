// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GAS/CGameplayAbility.h"
#include "GA_Freeze.generated.h"

/**
 * 
 */
UCLASS()
class UGA_Freeze : public UCGameplayAbility
{
	GENERATED_BODY()
	
public:
	UGA_Freeze();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	static FGameplayTag GetFreezeCastTag();

private:
	UPROPERTY(EditDefaultsOnly, Category = "Targeting")
	float TargetingRadius = 1000.f;

	UPROPERTY(EditDefaultsOnly, Category = "Targeting")
	float TargetingRange = 2000.f;

	UPROPERTY(EditDefaultsOnly, Category = "Targeting")
	TSubclassOf<class ATargetActor_GroundPick> TargetActorClass;

	UPROPERTY(EditDefaultsOnly, Category = "Anim")
	UAnimMontage* CastingMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Anim")
	UAnimMontage* TargetingMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Effect")
	TSubclassOf<UGameplayEffect> DamageEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Effect|Damage")
	TArray<float> BaseDamagePerLevel = { 60.f, 90.f, 120.f, 150.f, 180.f };

	UPROPERTY(EditDefaultsOnly, Category = "Effect|Damage")
	float ScalingRatio = 0.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Gameplay Cue")
	FGameplayTag FreezeGameplayCueTag;

	UPROPERTY(EditDefaultsOnly, Category = "Gameplay Cue")
	FGameplayTag FreezeTargetingGameplayCueTag;

	UFUNCTION()
	void TargetReceived(const FGameplayAbilityTargetDataHandle& TargetDataHandle);

	UFUNCTION()
	void TargetingCancelled(const FGameplayAbilityTargetDataHandle& TargetDataHandle);
};
