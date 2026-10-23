// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GAS/CGameplayAbility.h"
#include "GA_Dash.generated.h"

/**
 * 
 */
UCLASS()
class UGA_Dash : public UCGameplayAbility
{
	GENERATED_BODY()
	
public:
	UGA_Dash();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	static FGameplayTag GetDashStartTag();

private:
	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimMontage* CastMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimMontage* DashMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Targeting")
	float TargetDetectionRadius = 300.f;

	UPROPERTY(EditDefaultsOnly, Category = "Targeting")
	float TargetHitPushSpeed = 3000.f;
	
	UPROPERTY(EditDefaultsOnly, Category = "Targeting")
	TSubclassOf<class ATargetActor_Around> TargetActorClass;

	UPROPERTY(EditDefaultsOnly, Category = "Targeting")
	FName TargetActorAttachSocketName = "TargetDashCenter";

	UPROPERTY(EditDefaultsOnly, Category = "Targeting")
	TSubclassOf<class ATA_Directional > DirectionTargetActorClass;

	UPROPERTY(EditDefaultsOnly, Category = "Targeting")
	float DashDistance = 800.f;

	UPROPERTY(EditDefaultsOnly, Category = "Effect")
	TSubclassOf<UGameplayEffect> DashEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Effect")
	TSubclassOf<UGameplayEffect> DamageEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Effect|Damage")
	TArray<float> BaseDamagePerLevel = { 40.f, 65.f, 90.f, 115.f, 140.f };

	UPROPERTY(EditDefaultsOnly, Category = "Effect|Damage")
	float ScalingRatio = 0.5f;

	UPROPERTY(EditDefaultsOnly, Category = "GameplayCue")
	FGameplayTag LocalGameplayCueTag;

	bool bOriginalOrientRotationToMovement = true;
	bool bOriginalUseControllerRotationYaw = false;

	FActiveGameplayEffectHandle DashEffectHandle;

	FTimerHandle PushFowardInputTimerHandle;

	float OriginalMaxWalkSpeed;
	float OriginalGroundFriction;

	FVector ConfirmedDashDirection = FVector::ForwardVector;
	FVector DashStartLocation;

	UFUNCTION()
	void DirectionConfirmed(const FGameplayAbilityTargetDataHandle& TargetDataHandle);

	UFUNCTION()
	void DirectionCancelled(const FGameplayAbilityTargetDataHandle& TargetDataHandle);

	UFUNCTION()
	void StartDash(FGameplayEventData Payload);

	void PushFoward();

	class UCharacterMovementComponent* OwnerCharacterMovementComponent;

	UFUNCTION()
	void TargetReceived(const FGameplayAbilityTargetDataHandle& Data);
};
