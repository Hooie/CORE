// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Items/GA_Item_IronCitadel.h"
#include "AbilitySystemComponent.h"
#include "GAS/CAbilitySystemStatics.h"

UGA_Item_IronCitadel::UGA_Item_IronCitadel()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	ActivationBlockedTags.RemoveTag(UCAbilitySystemStatics::GetStunStatusTag());

	ActivationRequiredTags.AddTag(UCAbilitySystemStatics::GetCrowdControlledStatusTag());

	FAbilityTriggerData TriggerData;
	TriggerData.TriggerTag = UCAbilitySystemStatics::GetCrowdControlledStatusTag();
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::OwnedTagAdded;
	AbilityTriggers.Add(TriggerData);
}

void UGA_Item_IronCitadel::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if ( !HasAuthorityOrPredictionKey(ActorInfo, &ActivationInfo) )
	{
		K2_EndAbility();
		return;
	}

	if ( !K2_CommitAbility() )
	{
		K2_EndAbility();
		return;
	}

	if ( HasAuthority(&ActivationInfo) )
	{
		UAbilitySystemComponent* OwnerASC = GetAbilitySystemComponentFromActorInfo();
		UCAbilitySystemStatics::ApplyTimedShield(OwnerASC, OwnerASC, ShieldGrantEffect, ShieldBuffIconEffect, GetAbilityLevel(), ShieldDuration);
	}

	K2_EndAbility();
}
