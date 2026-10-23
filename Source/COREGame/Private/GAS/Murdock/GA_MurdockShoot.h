// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GAS/CGameplayAbility.h"
#include "GA_MurdockShoot.generated.h"

/**
 * 
 */
UCLASS()
class UGA_MurdockShoot : public UCGameplayAbility
{
	GENERATED_BODY()
	
public:
	UGA_MurdockShoot();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void InputReleased(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	static FGameplayTag GetShootTag();

	virtual bool GetBasicAttackDamagePreview(const UAbilitySystemComponent* SourceASC, float& OutRawDamage, bool& bOutIsMagical) const override;

private:
	UPROPERTY(EditDefaultsOnly, Category = "Anim")
	UAnimMontage* StandByMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Anim")
	UAnimMontage* ShootMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Shoot")
	TSubclassOf<UGameplayEffect> HitDamageEffeect;

	UPROPERTY(EditDefaultsOnly, Category = "Shoot")
	float ShootRange = 2500.f;

	UPROPERTY(EditDefaultsOnly, Category = "Shoot")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Pawn;

	UPROPERTY(EditDefaultsOnly, Category = "Shoot")
	FGameplayTagContainer TriggerGameplayCueTags;

	UPROPERTY(EditDefaultsOnly, Category = "Shoot")
	FName GunMuzzleSocketName = "Muzzle_01";

	UPROPERTY(EditDefaultsOnly, Category = "Shoot|Damage")
	float BaseDamage = 0.f;

	UPROPERTY(EditDefaultsOnly, Category = "Shoot|Damage")
	float ScalingRatio = 1.f;

	bool bIsHoldingFire = false;
	bool bIsShotInProgress = false;

	UFUNCTION()
	void StartShooting(FGameplayEventData Payload);

	UFUNCTION()
	void StopShooting(FGameplayEventData Payload);

	UFUNCTION()
	void FireHitScan(FGameplayEventData Payload);

	void TryFireNextShot();

	UFUNCTION()
	void OnShootMontageCompleted();

	UPROPERTY()
	class UAmmoComponent* CachedAmmoComponent;

	void OnAmmoComponentChanged(int32 Current, int32 Max);

	bool PenetratingTrace(const FVector& Start, const FVector& End, AActor* OwnerActor, FHitResult& OutHit, AActor*& OutHitActor) const;

	bool IsReloading() const;
};
