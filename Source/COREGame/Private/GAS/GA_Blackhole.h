// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GAS/CGameplayAbility.h"
#include "GA_Blackhole.generated.h"

/**
 * 
 */
UCLASS()
class UGA_Blackhole : public UCGameplayAbility
{
	GENERATED_BODY()
	
public:
	UGA_Blackhole();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;


private:
	UPROPERTY(EditDefaultsOnly, Category = "Targeting")
	float TargetAreaRadius = 300.f;

	UPROPERTY(EditDefaultsOnly, Category = "Targeting")
	float BlackholePullSpeed = 3000.f;

	UPROPERTY(EditDefaultsOnly, Category = "Targeting")
	float BlackholeDuration = 6.f;

	UPROPERTY(EditDefaultsOnly, Category = "Targeting")
	float TargetTraceRange = 2000.f;

	UPROPERTY(EditDefaultsOnly, Category = "Targeting")
	float BlowPushSpeed = 3000.f;

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimMontage* TargetingMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimMontage* HoldingBlackholeMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimMontage* FinalBlowMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Targeting")
	TSubclassOf<UGameplayEffect> SkillAimEffect;
	
	FActiveGameplayEffectHandle SkillAimEffectHandle;

	UPROPERTY(EditDefaultsOnly, Category = "Targeting")
	TSubclassOf<UGameplayEffect> FocusEffect;

	FActiveGameplayEffectHandle FocusEffectHandle;

	UPROPERTY(EditDefaultsOnly, Category = "Effect")
	TSubclassOf<UGameplayEffect> FinalBlowDamageEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Effect|Damage")
	TArray<float> BaseDamagePerLevel = { 220.f, 370.f, 520.f };

	UPROPERTY(EditDefaultsOnly, Category = "Effect|Damage")
	float ScalingRatio = 1.1f;

	UPROPERTY(EditDefaultsOnly, Category = "Effect|Damage")
	float MinHoldDamageMultiplier = 0.7;

	UPROPERTY(EditDefaultsOnly, Category = "Effect|Damage")
	float MaxHoldDamageMultiplier = 1.5f;

	float BlackholeSpawnTime = 0.f;

	UPROPERTY(EditDefaultsOnly, Category = "Targeting")
	TSubclassOf<class ATargetActor_GroundPick> TargetActorClass;

	UPROPERTY(EditDefaultsOnly, Category = "Targeting")
	TSubclassOf<class ATA_Blackhole> BlackholeTargetActorClass;

	UPROPERTY(EditDefaultsOnly, Category = "Targeting")
	FGameplayTag FinalBlowCueTag;

	UPROPERTY(EditDefaultsOnly, Category = "VFX")
	FGameplayTag HoldLoopVFXCueTag;

	UPROPERTY()
	class UAbilityTask_PlayMontageAndWait* PlayCastBlackholeMontageTask;

	UPROPERTY()
	class UAbilityTask_WaitTargetData* BlackholeTargetingTask;

	UPROPERTY()
	class ATA_Blackhole* SpawnedBlackholeTargetActor;

	UFUNCTION()
	void PlaceBlackhole(const FGameplayAbilityTargetDataHandle& TargetDataHandle);

	UFUNCTION()
	void PlacementCancelled(const FGameplayAbilityTargetDataHandle& TargetDataHandle);

	UFUNCTION()
	void FinalTargetReceived(const FGameplayAbilityTargetDataHandle& TargetDataHandle);

	void AddSkillAimEffect();
	void RemoveSkillAimEffect();

	void AddFocusEffect();
	void RemoveFocusEffect();

	void OnStunTagChanged(const FGameplayTag Tag, int32 NewCount);
};
