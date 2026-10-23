// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GAS/CGameplayAbility.h"
#include "GA_Minion_RangedAttack.generated.h"

/**
 * 
 */
UCLASS()
class UGA_Minion_RangedAttack : public UCGameplayAbility
{
	GENERATED_BODY()
	
public:
	UGA_Minion_RangedAttack();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

private:
	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimMontage* AttackMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Attack")
	TSubclassOf<class ARangedMinionProjectile> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, Category = "Attack")
	FName MuzzleSocketName = "Muzzle_01";

	UPROPERTY(EditDefaultsOnly, Category = "Attack")
	float ProjectileSpeed = 1100.f;

	UPROPERTY(EditDefaultsOnly, Category = "Attack")
	float ProjectileMaxDistance = 2000.f;

	UPROPERTY(EditDefaultsOnly, Category = "Attack")
	FName TargetBlackBoardName = "Target";

	UPROPERTY(EditDefaultsOnly, Category = "Attack|Damage")
	TSubclassOf<UGameplayEffect> HitDamageEffect;

	TWeakObjectPtr<AActor> CachedTarget;

	static FGameplayTag GetMinionAttackFireTag();

	UFUNCTION()
	void FireProjectile(FGameplayEventData PayLoad);

};
