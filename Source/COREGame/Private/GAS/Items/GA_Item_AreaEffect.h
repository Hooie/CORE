// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GAS/CGameplayAbility.h"
#include "GA_Item_AreaEffect.generated.h"

/**
 * 
 */
UCLASS()
class UGA_Item_AreaEffect : public UCGameplayAbility
{
	GENERATED_BODY()
	
public:
	UGA_Item_AreaEffect();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	
private:
	UPROPERTY(EditDefaultsOnly, Category = "AreaEffect")
	float Radius = 800.f;

	UPROPERTY(EditDefaultsOnly, Category = "AreaEffect")
	TEnumAsByte<ETeamAttitude::Type> TargetAttitude = ETeamAttitude::Friendly;

	UPROPERTY(EditDefaultsOnly, Category = "AreaEffect")
	bool bHeroesOnly = true;

	UPROPERTY(EditDefaultsOnly, Category = "AreaEffect")
	bool bIncludeSelf = true;

	UPROPERTY(EditDefaultsOnly, Category = "AreaEffect")
	TArray<TSubclassOf<UGameplayEffect>> EffectsToApply;

	UPROPERTY(EditDefaultsOnly, Category = "AreaEffect", meta = (Categories = "GameplayCue"))
	FGameplayTag CasterCueTag;

	UPROPERTY(EditDefaultsOnly, Category = "AreaEffect", meta = (Categories = "GameplayCue"))
	FGameplayTag TargetCueTag;

	void ApplyEffectsTo(UAbilitySystemComponent* OwnerASC, UAbilitySystemComponent* TargetASC, AActor* TargetActor);
};
