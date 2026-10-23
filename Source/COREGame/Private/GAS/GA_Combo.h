// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GAS/CGameplayAbility.h"
#include "GA_Combo.generated.h"

/**
 * 
 */
UCLASS()
class UGA_Combo : public UCGameplayAbility
{
	GENERATED_BODY()
	
public:
	UGA_Combo();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	static FGameplayTag GetComboChangedEventTag();
	static FGameplayTag GetComboChangedEventEndTag();
	static FGameplayTag GetComboTargetEventTag();

	virtual bool GetBasicAttackDamagePreview(const UAbilitySystemComponent* SourceASC, float& OutRawDamage, bool& bOutIsMagical) const override;

private:
	void SetupWaitComboInputPress();

	UFUNCTION()
	void HandleInputPress(float TimeWaited);

	void TryCommitCombo();

	UPROPERTY(EditDefaultsOnly, Category = "Gameplay Effect")
	TSubclassOf<UGameplayEffect> DamageEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Gameplay Effect|Damage")
	float BaseDamage = 0.f;

	UPROPERTY(EditDefaultsOnly, Category = "Gameplay Effect|Damage")
	float ScalingRatio = 1.f;

	UPROPERTY(EditDefaultsOnly, Category = "Animation")
	UAnimMontage* ComboMontage;

	UFUNCTION()
	void ComboChangedEventReceived(FGameplayEventData Data);

	UFUNCTION()
	void DoDamage(FGameplayEventData Data);

	FName NextComboName;

	UFUNCTION()
	void OnLifeStealProcess(FGameplayEventData EventData);

	bool IsBound() const;
};
