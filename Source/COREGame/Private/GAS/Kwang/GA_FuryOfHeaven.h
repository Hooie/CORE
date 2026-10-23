// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GAS/CGameplayAbility.h"
#include "GA_FuryOfHeaven.generated.h"

/**
 * 
 */
UCLASS()
class UGA_FuryOfHeaven : public UCGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_FuryOfHeaven();
	
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

private:
	UPROPERTY(EditDefaultsOnly, Category = "Montage")
	UAnimMontage* IntroMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Montage")
	UAnimMontage* FinalMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	TSubclassOf<UGameplayEffect> DamageEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Effects|Damage")
	TArray<float> BaseDamagePerLevel = { 250.f, 400.f, 550.f };

	UPROPERTY(EditDefaultsOnly, Category = "Effects|Damage")
	float ScalingRatio = 1.3f;

	UPROPERTY(EditDefaultsOnly, Category = "Effects")
	TSubclassOf<UGameplayEffect> BuffEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Targeting")
	float AttackRadius = 300.f;

	UFUNCTION()
	void IntroMontageCompleted();

	UFUNCTION()
	void RecallSwordEventReceived(FGameplayEventData Payload);

	UFUNCTION()
	void FinalDamageEventReceived(FGameplayEventData Payload);

	bool IsSwordInWorld();
	void TeleportToSword();

	FVector SavedSwordLocation;
	bool bShouldTeleport = false;

	float CachedMoveSpeed;
};
