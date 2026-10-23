// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GAS/CGameplayAbility.h"
#include "GA_Murdock_HotPursuit.generated.h"

/**
 * 
 */
UCLASS()
class UGA_Murdock_HotPursuit : public UCGameplayAbility
{
	GENERATED_BODY()
	
public:
	UGA_Murdock_HotPursuit();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	virtual void OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec) override;

private:
	UPROPERTY(EditDefaultsOnly, Category = "HotPursuit")
	TArray<float> DetectionRadiusPerLevel = { 2000.f, 3000.f, 4000.f, 5000.f, 6000.f };

	UPROPERTY(EditDefaultsOnly, Category = "HotPursuit")
	float LowHealthThresholdPercent = 0.25f;

	UPROPERTY(EditDefaultsOnly, Category = "HotPursuit")
	float MovingTowardDotThreshold = 0.5f;

	UPROPERTY(EditDefaultsOnly, Category = "HotPursuit")
	float CheckInterval = 0.2f;

	UPROPERTY(EditDefaultsOnly, Category = "Effect")
	TSubclassOf<UGameplayEffect> EnhanceMovementEffect;

	FActiveGameplayEffectHandle EnhanceMovementEffectHandle;

	FTimerHandle CheckTimerHandle;

	int32 CurrentAbilityLevel = 0;

	UFUNCTION()
	void OnAbilitySpecDirted(const FGameplayAbilitySpec& Spec);

	void StartPeriodicCheck();
	void StopPeriodicCheck();
	void PeriodicCheck();

	bool FindQualifyingLowHealthEnemy(float Radius, FVector& OutEnemyLocation) const;
};
