// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GAS/CGameplayAbility.h"
#include "GA_JudgementOfHeaven.generated.h"

/**
 *
 */
UCLASS()
class UGA_JudgementOfHeaven : public UCGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_JudgementOfHeaven();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	static FGameplayTag GetFinalTag();
	static FGameplayTag GetThrowTag();

private:
	UPROPERTY(EditDefaultsOnly, Category = "Targeting")
	float TargetTraceRange = 1000.f;

	UPROPERTY(EditDefaultsOnly, Category = "Targeting")
	float TargetAreaRadius = 300.f;

	UPROPERTY(EditDefaultsOnly, Category = "Targeting")
	float SwordPullSpeed = 3000.f;

	UPROPERTY(EditDefaultsOnly, Category = "Targeting")
	float SwordDuration = 6.f;

	UPROPERTY(EditDefaultsOnly, Category = "Targeting")
	float SwordBindInterval = 5.f;

	UPROPERTY(EditDefaultsOnly, Category = "Targeting|Damage")
	TArray<float> SwordBaseDamagePerLevel = { 80.f, 100.f, 120.f, 150.f, 180.f};

	UPROPERTY(EditDefaultsOnly, Category = "Targeting|Damage")
	float SwordScalingRatio = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimMontage* TargetingMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimMontage* ThrowingSwordMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimMontage* FinalMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Targeting")
	TSubclassOf<UGameplayEffect> SwordDamageEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Targeting")
	TSubclassOf<UGameplayEffect> SkillAimEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Targeting")
	TSubclassOf<UGameplayEffect> DisableFuryEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Targeting")
	TSubclassOf<UGameplayEffect> NoWeaponEffect;

	FActiveGameplayEffectHandle SkillAimEffectHandle;
	FActiveGameplayEffectHandle NoWeaponEffectHandle;

	UPROPERTY(EditDefaultsOnly, Category = "Targeting")
	TSubclassOf<class ATargetActor_GroundPick> TargetActorClass;

	UPROPERTY(EditDefaultsOnly, Category = "Targeting")
	TSubclassOf<class ATA_Sword> SwordTargetActorClass;

	UPROPERTY()
	class UAbilityTask_PlayMontageAndWait* PlayCastSwordMontageTask;

	UPROPERTY()
	class UAbilityTask_WaitTargetData* AfterThrowingSwordTask;

	UPROPERTY()
	class UAbilityTask_WaitInputPress* WaitRetriggerTask;

	UFUNCTION()
	void OnRetriggerInputPressed(float TimeWaited);

	UPROPERTY()
	class ATA_Sword* SwordTargetActor;

	UFUNCTION()
	void PlaceSword(const FGameplayAbilityTargetDataHandle& TargetDataHandle);

	UFUNCTION()
	void PlacementCancelled(const FGameplayAbilityTargetDataHandle& TargetDataHandle);

	UFUNCTION()
	void FinalTargetReceived(const FGameplayAbilityTargetDataHandle& TargetDataHandle);

	void AddSkillAimEffect();
	void RemoveSkillAimEffect();
	void AddNoWeaponEffect();
	void RemoveNoWeaponEffect();

	void SetSwordVisibility(bool bVisible);

	UFUNCTION()
	void FinalTagReceived(FGameplayEventData Payload);

	UFUNCTION()
	void ThrowTagReceived(FGameplayEventData Payload);

	void DisableFury();
};
