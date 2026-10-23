// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GenericTeamAgentInterface.h"
#include "CGameplayAbility.generated.h"

/**
 * 
 */
UCLASS()
class UCGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UCGameplayAbility();
	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr, OUT FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	
	virtual bool GetBasicAttackDamagePreview(const UAbilitySystemComponent* SourceASC, float& OutRawDamage, bool& bOutIsMagical) const { return false; }

protected:
	AActor* GetAimTarget(float AimDistance, ETeamAttitude::Type TeamAttitude) const;

	class UAnimInstance* GetOwnerAnimInstance() const;
	TArray<FHitResult> GetHitResultFromSweepLocationTargetData(const FGameplayAbilityTargetDataHandle& TargetDataHandle, float SphereSweepRadius = 30.f, ETeamAttitude::Type TargetTeam = ETeamAttitude::Hostile, bool bDrawDebug = false, bool bIgnoreSelf = true) const;

	FGameplayEffectSpecHandle MakeScaledDamageSpec(TSubclassOf<UGameplayEffect> DamageEffect, float BaseDamage, float ScalingRatio, int Level = 1);
	FGameplayEffectSpecHandle MakeScaledDamageSpec(TSubclassOf<UGameplayEffect> DamageEffect, float BaseDamage, float StrengthRatio, float IntelligenceRatio, int Level); 
	
	float GetLevelIndexedDamage(const TArray<float>& DamagePerLevel, int32 Level) const;

	float GetScaledAmount(const TArray<float>& BasePerLevel, int32 Level, float StrengthRatio, float IntelligenceRatio) const;

	UFUNCTION()
	FORCEINLINE bool ShouldDrawDebug() const { return bShouldDrawDebug; }

	void PushSelf(const FVector& PushVel);

	void PushTarget(AActor* Target, const FVector& PushVel);
	void PushTargets(const TArray<AActor*>& Targets, const FVector& PushVel);
	void PushTargets(const FGameplayAbilityTargetDataHandle& TargetHandle, const FVector& PushVel);

	// bWithZ = true이면 z방향으로 밀기
	void PushTargetsFromOwnerLocation(const TArray<AActor*>& Targets, float PushSpeed, bool bWithZ = true);
	// bWithZ = false이면 위 방향으로 X
	void PushTargetsFromOwnerLocation(const FGameplayAbilityTargetDataHandle& TargetDataHandle, float PushSpeed, bool bWithZ = true);

	// Z bWithZ = true이면 z방향으로 밀기
	void PushTargetsFromLocation(const TArray<AActor*>& Targets, const FVector& FromLocation, float PushSpeed, bool bWithZ = true);
	// bWithZ = false이면 위 방향으로 X
	void PushTargetsFromLocation(const FGameplayAbilityTargetDataHandle& TargetDataHandle, const FVector& FromLocation, float PushSpeed, bool bWithZ = true);

	void PlayMontageLocally(UAnimMontage* MontageToPlay);
	void StopMontageAfterCurrentSection(UAnimMontage* MontageToStop);

	ACharacter* GetOwningAvatarCharacter();

	void ApplyGameplayEffectToHitResultActor(const FHitResult& HitResult, TSubclassOf<UGameplayEffect> GameplayEffect, int Level = 1);
	void ApplyGameplayEffectToHitResultActor(const FHitResult& HitResult, FGameplayEffectSpecHandle EffectSpecHandle);

	FGenericTeamId GetOwnerTeamId() const;

	bool IsActorTeamAttitude(const AActor* OtherActor, ETeamAttitude::Type TeamAttitude) const;

	void SendLocalGameplayEvent(const FGameplayTag& EventTag, const FGameplayEventData& EventData);

	FORCEINLINE TSubclassOf<UGameplayEffect> GetLifeStealHealEffect() { return LifeStealHealEffect; }

	class UGameplayWidget* GetOwningGameplayWidget() const;

private:
	UPROPERTY(EditDefaultsOnly, Category = "Debug")
	bool bShouldDrawDebug = false;

	UPROPERTY(EditDefaultsOnly, Category = "Effect")
	TSubclassOf<UGameplayEffect> LifeStealHealEffect;

	UPROPERTY()
	class ACharacter* AvatarCharacter;
};
