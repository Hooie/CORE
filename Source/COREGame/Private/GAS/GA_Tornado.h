// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GAS/CGameplayAbility.h"
#include "GA_Tornado.generated.h"

/**
 * 
 */
UCLASS()
class UGA_Tornado : public UCGameplayAbility
{
	GENERATED_BODY()
	
public:
	UGA_Tornado();
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

private:
	UPROPERTY(EditDefaultsOnly, Category = "Anim")
	UAnimMontage* TornadoMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Effect")
	TSubclassOf<UGameplayEffect> DamageEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Effect|Damage")
	TArray<float> BaseDamagePerLevel = { 20.f, 30.f, 40.f };

	UPROPERTY(EditDefaultsOnly, Category = "Effect|Damage")
	float ScalingRatio = 0.3f;

	UPROPERTY(EditDefaultsOnly, Category = "Effect")
	float HitPushSpeed = 1000.f;

	UPROPERTY(EditDefaultsOnly, Category = "Effect")
	float TornadoDuration = 4.f;

	UFUNCTION()
	void TornadoDamageEventReceived(FGameplayEventData Payload);
};
