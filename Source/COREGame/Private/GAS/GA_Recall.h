// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GAS/CGameplayAbility.h"
#include "GA_Recall.generated.h"

UCLASS()
class UGA_Recall : public UCGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Recall();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

private:
	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimMontage* RecallMontage;

	UPROPERTY(EditDefaultsOnly, Category = "VFX");
	FGameplayTag RecallCompletedVFXCueTag;

	UPROPERTY(EditDefaultsOnly, Category = "Recall")
	float RecallDuration = 8.f;

	UPROPERTY(EditDefaultsOnly, Category = "Effect")
	TSubclassOf<class UGameplayEffect> RecallEffect;

	FActiveGameplayEffectHandle RecallEffectHandle;

	FTimerHandle RecallCompletionTimerHandle;
	void OnRecallTimerCompleted();

	UFUNCTION()
	void OnRecallInterrupted();

	UFUNCTION()
	void OnDamagedDuringRecall(FGameplayEventData Payload);
};