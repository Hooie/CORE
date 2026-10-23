// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GAS/CGameplayAbility.h"
#include "GA_Shoot.generated.h"

/**
 * 
 */
UCLASS()
class UGA_Shoot : public UCGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Shoot();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void InputReleased(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	virtual bool GetBasicAttackDamagePreview(const UAbilitySystemComponent* SourceASC, float& OutRawDamage, bool& bOutIsMagical) const override;

private:
	static FGameplayTag GetShootTag();

	UPROPERTY(EditDefaultsOnly, Category = "Shoot")
	TSubclassOf<UGameplayEffect> ProjectileHitEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Shoot")
	float ShootProjectileSpeed = 2500.f;

	UPROPERTY(EditDefaultsOnly, Category = "Shoot")
	float ShootProjectileRange = 2500.f;

	UPROPERTY(EditDefaultsOnly, Category = "Shoot")
	TSubclassOf<class AProjectileActor> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, Category = "Shoot|Damage")
	float BaseDamage = 0.f;

	UPROPERTY(EditDefaultsOnly, Category = "Shoot|Damage")
	float StrengthRatio = 1.f;

	UPROPERTY(EditDefaultsOnly, Category = "Shoot|Damage")
	float IntelligenceRatio = 0.15f;

	UPROPERTY(EditDefaultsOnly, Category = "Anim")
	UAnimMontage* ShootMontage;

	UFUNCTION()
	void StartShooting(FGameplayEventData PayLoad);

	UFUNCTION()
	void StopShooting(FGameplayEventData PayLoad);

	UFUNCTION()
	void ShootProjectile(FGameplayEventData PayLoad);

	AActor* GetAimTargetIfValid() const;

	UPROPERTY()
	AActor* AimTarget;

	UPROPERTY()
	UAbilitySystemComponent* AimTargetAbilitySystemComponent;

	FTimerHandle AimTargetCheckTimerHandle;

	void FindAimTarget();

	UPROPERTY(EditDefaultsOnly, Category = "Target")
	float AimTargetCheckInterval = 0.1f;

	void StartAimTargetCheckTimer();
	void StopAimTargetCheckTimer();

	bool HasValidTarget() const;
	bool IsTargetInRange() const;

	void TargetDeadTagUpdated(const FGameplayTag Tag, int32 NewCount);
};