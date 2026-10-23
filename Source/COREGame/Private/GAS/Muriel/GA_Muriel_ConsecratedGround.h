// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GAS/CGameplayAbility.h"
#include "GA_Muriel_ConsecratedGround.generated.h"

/**
 * 
 */
UCLASS()
class UGA_Muriel_ConsecratedGround : public UCGameplayAbility
{
	GENERATED_BODY()
	
public:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

private:
	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimMontage* TargetingMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimMontage* CastMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Targeting")
	TSubclassOf<class ATA_ConsecratedGroundPreview> PreviewTargetActorClass;

	UPROPERTY(EditDefaultsOnly, Category = "Targeting")
	TSubclassOf<class AConsecratedGroundZone> ZoneClass;

	UPROPERTY(EditDefaultsOnly, Category = "Targeting")
	float MaxRange = 3000.f;

	UPROPERTY(EditDefaultsOnly, Category = "Targeting")
	float ZoneRadius = 1000.f;

	UPROPERTY(EditDefaultsOnly, Category = "Targeting")
	float ZoneDuration = 8.f;

	UPROPERTY(EditDefaultsOnly, Category = "Effect")
	float ShieldDuration = 7.f;

	UPROPERTY(EditDefaultsOnly, Category = "Effect")
	TArray<float> ShieldBaseAmountPerLevel = { 50.f, 80.f, 115.f, 150.f, 200.f };

	UPROPERTY(EditDefaultsOnly, Category = "Effect")
	float ShieldIntelligenceRatio = 0.4f;

	UPROPERTY(EditDefaultsOnly, Category = "Effect")
	TSubclassOf<UGameplayEffect> ShieldEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Effect")
	TSubclassOf<UGameplayEffect> ShieldBuffIconEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Targeting")
	TSubclassOf<UGameplayEffect> SkillAimEffect;

	FActiveGameplayEffectHandle SkillAimEffectHandle;

	UPROPERTY(EditDefaultsOnly, Category = "Targeting|Damage")
	TArray<float> BaseDamagePerLevel = { 70.f, 100.f, 130.f, 160.f, 190.f };

	UPROPERTY(EditDefaultsOnly, Category = "Targeting|Damage")
	float ScalingRatio = 0.5f;

	UPROPERTY(EditDefaultsOnly, Category = "Targeting|Damage")
	TSubclassOf<UGameplayEffect> DamageEffect;

	UPROPERTY()
	class AConsecratedGroundZone* SpawnedZone;

	void AddSkillAimEffect();
	void RemoveSkillAimEffect();

	UFUNCTION()
	void OnTargetConfirmed(const FGameplayAbilityTargetDataHandle& TargetDataHandle);
	
	UFUNCTION()
	void OnTargetCancelled(const FGameplayAbilityTargetDataHandle& TargetDataHandle);

	UFUNCTION()
	void OnRetriggerInputPressed(float TimeWaited);

};
