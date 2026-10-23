// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GAS/CGameplayAbility.h"
#include "GA_Muriel_Serenity.generated.h"

/**
 * 
 */
UCLASS()
class UGA_Muriel_Serenity : public UCGameplayAbility
{
	GENERATED_BODY()

public:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	static FGameplayTag GetFireTag();

private:
	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimMontage* TargetingMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimMontage* ShootMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Targeting")
	float ProjectileSpeed = 4000.f;

	UPROPERTY(EditDefaultsOnly, Category = "Targeting")
	float ProjectileMaxDistance = 5000.f;

	UPROPERTY(EditDefaultsOnly, Category = "Targeting")
	TSubclassOf<class ASerenityProjectile> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, Category = "Targeting")
	FName MuzzleSocketName = "Muzzle_01";

	UPROPERTY(EditDefaultsOnly, Category = "Targeting|Damage")
	TArray<float> BaseDamagePerLevel = { 50.f, 80.f, 120.f, 160.f, 200.f };

	UPROPERTY(EditDefaultsOnly, Category = "Targeting|Damage")
	float ScalingRatio = 0.6f;

	UPROPERTY(EditDefaultsOnly, Category = "Targeting|Damage")
	TSubclassOf<UGameplayEffect> HitEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Targeting")
	TSubclassOf<UGameplayEffect> BasicAttackAimEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Targeting")
	TSubclassOf<UGameplayEffect> CrosshairEffect;

	FActiveGameplayEffectHandle BasicAttackAimEffectHandle;
	FActiveGameplayEffectHandle CrosshairEffectHandle;

	void AddBasicAttackAimEffect();
	void RemoveBasicAttackAimEffect();

	UFUNCTION()
	void OnConfirmed();

	UFUNCTION()
	void OnCancelled();

	UFUNCTION()
	void FireProjectile(FGameplayEventData PayLoad);
};
