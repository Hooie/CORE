// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GAS/CGameplayAbility.h"
#include "GA_Muriel_Alacrity.generated.h"

/**
 * 
 */
UCLASS()
class UGA_Muriel_Alacrity : public UCGameplayAbility
{
	GENERATED_BODY()
	
public:
	UGA_Muriel_Alacrity();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	
	static FGameplayTag GetAlacrityFireTag();

private:
	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimMontage* TargetingMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimMontage* ShootMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Shoot")
	TSubclassOf<class AAllyShootProjectile> ProjectileClass;

	UPROPERTY(EditDefaultsOnly, Category = "Shoot")
	float ProjectileSpeed = 2000.f;

	UPROPERTY(EditDefaultsOnly, Category = "Shoot")
	float ProjectileMaxDistance = 3000.f;

	UPROPERTY(EditDefaultsOnly, Category = "Shoot|Targeting")
	bool bUseAllyTargeting = false;

	UPROPERTY(EditDefaultsOnly, Category = "Effect")
	TSubclassOf<UGameplayEffect> MoveSpeedBuffEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Effect")
	TSubclassOf<UGameplayEffect> ShieldEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Effect")
	TSubclassOf<UGameplayEffect> ShieldBuffIconEffect;
	
	UPROPERTY(EditDefaultsOnly, Category = "Effect")
	float ShieldDuration = 5.f;

	UPROPERTY(EditDefaultsOnly, Category = "Effect")
	TArray<float> ShieldBaseAmountPerLevel = { 60.f, 85.f, 110.f, 135.f, 160.f };
	
	UPROPERTY(EditDefaultsOnly, Category = "Effect")
	float ShieldIntelligenceRatio = 0.4f;

	UPROPERTY(EditDefaultsOnly, Category = "Shoot")
	FName MuzzleSocketName = "Muzzle_01";

	UFUNCTION()
	void FireProjectile(FGameplayEventData PayLoad);

	UFUNCTION()
	void OnConfirmed();

	UFUNCTION()
	void OnCancelled();

	UFUNCTION()
	void OnAllyHitByProjectile(AActor* HitActor, UAbilitySystemComponent* HitASC);
};
