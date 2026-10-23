// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GAS/CGameplayAbility.h"
#include "GA_Murdock_JudgementBlast.generated.h"

/**
 * 
 */
UCLASS()
class UGA_Murdock_JudgementBlast : public UCGameplayAbility
{
	GENERATED_BODY()
	
public:
	UGA_Murdock_JudgementBlast();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	static FGameplayTag GetBlastFireTag();
	
private:
	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimMontage* TargetingMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimMontage* ShootMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Targeting")
	float ShootingReadyDelay = 1.f;

	UPROPERTY(EditDefaultsOnly, Category = "Targeting")
	FName MuzzleSocketName = "Muzzle_01";

	UPROPERTY(EditDefaultsOnly, Category = "Targeting")
	float BlastRange = 10000.f;

	UPROPERTY(EditDefaultsOnly, Category = "Targeting")
	float BlastRadius = 150.f;

	UPROPERTY(EditDefaultsOnly, Category = "Targeting|Damage")
	TArray<float> BaseDamagePerLevel = { 200.f, 350.f, 500.f };

	UPROPERTY(EditDefaultsOnly, Category = "Targeting|Damage")
	float ScalingRatio = 1.f;

	UPROPERTY(EditDefaultsOnly, Category = "Targeting|Damage")
	TSubclassOf<UGameplayEffect> HitDamageEffect;

	FVector ConfirmedAimDirection = FVector::ForwardVector;

	FTimerHandle TargetingDelayHandle;

	UFUNCTION()
	void OnConfirmed();

	UFUNCTION()
	void FireBlast(FGameplayEventData Payload);

	bool bLaserSightActive = false;

	void StartLaserSight();
	void StopLaserSight();

	void StartShootingPhase();
};
