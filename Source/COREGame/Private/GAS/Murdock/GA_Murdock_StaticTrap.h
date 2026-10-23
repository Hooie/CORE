// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GAS/CGameplayAbility.h"
#include "GA_Murdock_StaticTrap.generated.h"

/**
 * 
 */
UCLASS()
class UGA_Murdock_StaticTrap : public UCGameplayAbility
{
	GENERATED_BODY()
	
public:
	UGA_Murdock_StaticTrap();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	static FGameplayTag GetThrowTag();

private:
	UPROPERTY(EditDefaultsOnly, Category = "Targeting")
	float TrapThowRange = 700.f;

	UPROPERTY(EditDefaultsOnly, Category = "Targeting")
	float TrapAreaRadius = 100.f;

	UPROPERTY(EditDefaultsOnly, Category = "Targeting")
	TSubclassOf<UGameplayEffect> SkillAimEffect;

	FActiveGameplayEffectHandle SkillAimEffectHandle;

	UPROPERTY(EditDefaultsOnly, Category = "Targeting")
	TSubclassOf<class ATargetActor_GroundPick> TargetActorClass;

	UPROPERTY(EditDefaultsOnly, Category = "Targeting")
	float TrapLifetime = 30.f;

	UPROPERTY(EditDefaultsOnly, Category = "Targeting|Damage")
	TArray<float> TrapBaseDamagePerLevel = { 30.f, 50.f, 70.f, 90.f, 120.f };

	UPROPERTY(EditDefaultsOnly, Category = "Targeting|Damage")
	float TrapDamageScalingRatio = 0.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Targeting|Damage")
	TSubclassOf<UGameplayEffect> TrapDamageEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Targeting")
	TSubclassOf<UGameplayEffect> BindEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Targeting")
	TSubclassOf<class ATA_StaticTrap> TrapActorClass;

	UPROPERTY(EditDefaultsOnly, Category = "Anim")
	UAnimMontage* TargetingMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Anim")
	UAnimMontage* ThrowingMontage;

	UPROPERTY()
	class ATA_StaticTrap* SpawnedTrapActor;

	FVector PendingTrapSpawnLocation;

	UPROPERTY()
	class UAbilityTask_PlayMontageAndWait* PlayTargetingMontage;

	UFUNCTION()
	void PlaceTrap(const FGameplayAbilityTargetDataHandle& Data);

	UFUNCTION()
	void PlacementCancelled(const FGameplayAbilityTargetDataHandle& Data);

	void AddSkillAimEffect();
	void RemoveSkillAimEffect();

	UFUNCTION()
	void ThrowTagReceived(FGameplayEventData Payload);
};
