// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GAS/CGameplayAbility.h"
#include "GA_Muriel_ReversalOfFortune.generated.h"

/**
 * 
 */
UCLASS()
class UGA_Muriel_ReversalOfFortune : public UCGameplayAbility
{
	GENERATED_BODY()

public:
	UGA_Muriel_ReversalOfFortune();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

private:
	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimMontage* TargetingMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimMontage* CastMontage;

	UPROPERTY(EditDefaultsOnly, Category = "Targeting")
	bool bUseTargetingReadyEvent = false;

	UPROPERTY(EditDefaultsOnly, Category = "Targeting")
	float TargetingReadyDelay = 1.f;

	UPROPERTY(EditDefaultsOnly, Category = "Targeting")
	TSubclassOf<class ATA_AllySelector> SelectorClass;

	UPROPERTY(EditDefaultsOnly, Category = "Ascent")
	float AscentHeight = 2500.f;

	// TargetingMontage/CastMontage의 상승 구간 길이와 맞춰야 함
	UPROPERTY(EditDefaultsOnly, Category = "Ascent")
	float AscentDuration = 1.6f; 

	UPROPERTY(EditDefaultsOnly, Category = "Ascent")
	class UCurveFloat* AscentTimeCurve;

	// 애님 노티파이 스테이트와 이름 일치 필요
	UPROPERTY(EditDefaultsOnly, Category = "Descent")
	FName WarpTargetName = "ReversalLandTarget"; 

	UPROPERTY(EditDefaultsOnly, Category = "Descent")
	float LandingOffsetDistance = 150.f;

	UPROPERTY(EditDefaultsOnly, Category = "Effect")
	TSubclassOf<UGameplayEffect> ShieldEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Effect")
	TSubclassOf<UGameplayEffect> ShieldBuffIconEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Effect")
	float ShieldDuration = 6.f;

	UPROPERTY(EditDefaultsOnly, Category = "Effect")
	TArray<float> ShieldBaseAmountPerLevel = { 200.f, 300.f, 400.f };

	UPROPERTY(EditDefaultsOnly, Category = "Effect")
	float ShieldIntelligenceRatio = 0.6f;

	UPROPERTY(EditDefaultsOnly, Category = "Effect")
	float LaunchRadius = 600.f;

	UPROPERTY(EditDefaultsOnly, Category = "Effect")
	float LaunchZVelocity = 1200.f;

	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	bool bLockCameraOnTargetDuringDescent = true;

	// 0이면 즉시 고정, 값이 클수록 빠르게 타겟을 향함
	UPROPERTY(EditDefaultsOnly, Category = "Camera")
	float CameraLockInterpSpeed = 8.f;

	AActor* TargetAlly;
	FVector AscentStartLocation;
	FVector AscentTargetLocation;

	FVector FixedAirborneCenter;
	FVector FixedLandingLocation;

	UFUNCTION()
	void OnTargetConfirmed(const FGameplayAbilityTargetDataHandle& TargetDataHandle);

	UFUNCTION()
	void OnTargetCancelled(const FGameplayAbilityTargetDataHandle& TargetDataHandle);

	void StartTargetingPhase();

	UFUNCTION()
	void OnTargetingReadyEvent(FGameplayEventData PayLoad);

	static FGameplayTag GetTargetingReadyTag();

	void CalculateFixedLandingData();

	uint16 AscentRootMotionSourceID = 0;

	void StartAscent();
	void StopAscent();
	void SetAnimRootMotionIgnored(bool bIgnore);

	bool bAnimRootMotionIgnored = false;
	uint8 SavedRootMotionMode = 0;

	bool bCameraLockActive = false;
	bool bSavedCameraInputBlocked = false;

	void StartCameraLock();
	void StopCameraLock();
	void UpdateCameraLock();

	UFUNCTION()
	void OnDescentStarted(FGameplayEventData PayLoad);

	UFUNCTION()
	void OnLanded(FGameplayEventData PayLoad);

	static FGameplayTag GetDescentStartTag();
	static FGameplayTag GetLandTag();
};
