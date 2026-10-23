// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Items/GA_Item_PurifyingChalice.h"
#include "AbilitySystemComponent.h"
#include "GAS/CAbilitySystemStatics.h"

UGA_Item_PurifyingChalice::UGA_Item_PurifyingChalice()
{
	CleanseTags.AddTag(UCAbilitySystemStatics::GetCrowdControlledStatusTag());
	CleanseTags.AddTag(UCAbilitySystemStatics::GetStunStatusTag());
	CleanseTags.AddTag(UCAbilitySystemStatics::GetBindStatusTag());
	CleanseTags.AddTag(UCAbilitySystemStatics::GetDisarmedTag());
}

void UGA_Item_PurifyingChalice::ApplyToTarget(UAbilitySystemComponent* OwnerASC, UAbilitySystemComponent* TargetASC, AActor* TargetActor)
{
	if ( !CleanseTags.IsEmpty() )
	{
		TargetASC->RemoveActiveEffectsWithGrantedTags(CleanseTags);
	}

	if ( HealEffect )
	{
		FGameplayEffectSpecHandle HealSpec = MakeOutgoingGameplayEffectSpec(HealEffect, GetAbilityLevel());
		if ( HealSpec.IsValid() )
		{
			OwnerASC->ApplyGameplayEffectSpecToTarget(*HealSpec.Data.Get(), TargetASC);
		}
	}
}