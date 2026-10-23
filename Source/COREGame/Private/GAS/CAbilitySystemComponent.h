// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffectTypes.h"
#include "GAS/CGameplayAbilityTypes.h"
#include "GAS/PA_AbilitySystemGenerics.h"

#include "CAbilitySystemComponent.generated.h"

/**
 * 
 */
UCLASS()
class UCAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	UCAbilitySystemComponent();
	void InitializeBaseAttributes();
	void ServerSideInit();
	void ApplyFullStatEffect();

	void ApplyMinionTimeScaling(const FMinionScalingConfig& Config, float ElapsedSeconds);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_UpgradeAbilityWithID(ECAbilityInputID InputID);

	UFUNCTION(Client, Reliable)
	void Client_AbilitySpecLevelUpdated(FGameplayAbilitySpecHandle Handle, int NewLevel);

	bool IsAtMaxLevel() const;

	const TMap<ECAbilityInputID, TSubclassOf<UGameplayAbility>>& GetAbilities() const;

private:
	void ApplyInitialEffects();
	void GiveInitialAbilities();

	void AuthApplyGameplayEffect(TSubclassOf<UGameplayEffect> GameplayEffect, int Level = 1);
	void AuthApplyGameplayEffectWithDuration(TSubclassOf<UGameplayEffect> GameplayEffect, float Duration, int Level = 1);

	void HealthUpdated(const FOnAttributeChangeData& ChangeData);
	void ManaUpdated(const FOnAttributeChangeData& ChangeData);
	void ExperienceUpdated(const FOnAttributeChangeData& ChangeData);

	UPROPERTY(EditDefaultsOnly, Category = "Gameplay Abilities")
	TMap<ECAbilityInputID, TSubclassOf<UGameplayAbility>> Abilities;
	
	UPROPERTY(EditDefaultsOnly, Category = "Gameplay Abilities")
	TMap<ECAbilityInputID, TSubclassOf<UGameplayAbility>> BasicAbilities;

	UPROPERTY(EditDefaultsOnly, Category = "Gameplay Abilities")
	UPA_AbilitySystemGenerics* AbilitySystemGenerics;

	FHeroBaseStats CachedBaseStats;
	bool bHasCachedBaseStats = false;
};
