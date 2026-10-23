// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GAS/CGameplayAbility.h"
#include "GA_Murdock_Shotgun.generated.h"

/**
 * 
 */
UCLASS()
class UGA_Murdock_Shotgun : public UCGameplayAbility
{
	GENERATED_BODY()
	
public:
	UGA_Murdock_Shotgun();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	
	static FGameplayTag GetShotgunFireTag();

private:
	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimMontage* TargetingMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimMontage* ShootMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Targeting")
	FName ShotgunFireSocketName = "Muzzle_03";
	
	UPROPERTY(EditDefaultsOnly, Category = "Targeting")
	int32 ShotgunSpreadCount = 6;

	UPROPERTY(EditDefaultsOnly, Category = "Targeting")
	float ShotgunSpreadAngle = 100.f;

	UPROPERTY(EditDefaultsOnly, Category = "Targeting")
	float ShotgunShotRange = 1000.f;

	UPROPERTY(EditDefaultsOnly, Category = "Targeting")
	TEnumAsByte<ECollisionChannel> TraceChannel = ECC_Pawn;

	UPROPERTY(EditDefaultsOnly, Category = "Targeting|Damage")
	TSubclassOf<UGameplayEffect> HitDamageEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Targeting|Damage")
	TArray<float> BaseDamagePerShotPerLevel = { 15.f, 17.5f, 20.f, 22.5f, 25.f };

	UPROPERTY(EditDefaultsOnly, Category = "Targeting|Damage")
	float ScalingRatio = 0.1f;

	UPROPERTY(EditDefaultsOnly, Category = "Targeting|Knockback")
	float PushSpeedPerHit = 150.f;

	UPROPERTY(EditDefaultsOnly, Category = "Targeting|Knockback")
	bool bPushWithZ = true;

	UFUNCTION()
	void OnConfirmed();

	UFUNCTION()
	void OnCancelled();

	bool PenetratingTrace(const FVector& Start, const FVector& End, AActor* OwnerActor, FHitResult& OutHit, AActor*& OutHitActor) const;

	UFUNCTION()
	void FireShotgun(FGameplayEventData Payload);
};
