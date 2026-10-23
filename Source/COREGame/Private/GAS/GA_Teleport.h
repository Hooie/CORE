// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GAS/CGameplayAbility.h"
#include "GA_Teleport.generated.h"

/**
 * 
 */
UCLASS()
class UGA_Teleport : public UCGameplayAbility
{
	GENERATED_BODY()
	
public:
	UGA_Teleport();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	static FGameplayTag GetTeleportTag();

private:
	UPROPERTY(EditDefaultsOnly, Category = "Anim")
	UAnimMontage* TargetingMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Anim")
	UAnimMontage* TeleportMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Effect")
	TSubclassOf<UGameplayEffect> DamageEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Effect|Damage")
	TArray<float> BaseDamagePerLevel = { 40.f, 60.f, 80.f, 100.f, 120.f };

	UPROPERTY(EditDefaultsOnly, Category = "Effect|Damage")
	float ScalingRatio = 0.4f;

	UPROPERTY(EditDefaultsOnly, Category = "Effect")
	float TeleportCastRange = 3000.f;

	UPROPERTY(EditDefaultsOnly, Category = "Effect")
	float TeleportLandTargetPushSpeed = 300.f;

	UPROPERTY(EditDefaultsOnly, Category = "Effect")
	float TeleportAreaRadius = 1000.f;

	UPROPERTY(EditDefaultsOnly, Category = "Targeting")
	TSubclassOf<class ATargetActor_GroundPick> GroundPickTargetActorClass;

	FGameplayAbilityTargetDataHandle TeleportTargetDataHandle;

	UFUNCTION()
	void GroundPickTargetReceived(const FGameplayAbilityTargetDataHandle& TargetDataHandle);

	UFUNCTION()
	void GroundPickCancelled(const FGameplayAbilityTargetDataHandle& TargetDataHandle);

	UFUNCTION()
	void Teleport(FGameplayEventData Payload);
};
