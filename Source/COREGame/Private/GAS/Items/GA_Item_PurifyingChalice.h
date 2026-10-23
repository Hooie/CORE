// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GAS/Items/GA_Item_AllyTargetBase.h"
#include "GA_Item_PurifyingChalice.generated.h"

/**
 * 
 */
UCLASS()
class UGA_Item_PurifyingChalice : public UGA_Item_AllyTargetBase
{
	GENERATED_BODY()
	
public:
	UGA_Item_PurifyingChalice();

protected:
	virtual void ApplyToTarget(UAbilitySystemComponent* OwnerASC, UAbilitySystemComponent* TargetASC, AActor* TargetActor) override;

private:
	UPROPERTY(EditDefaultsOnly, Category = "PurifyingChalice")
	FGameplayTagContainer CleanseTags;

	UPROPERTY(EditDefaultsOnly, Category = "PurifyingChalice")
	TSubclassOf<UGameplayEffect> HealEffect;
};
