// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Kwang/GA_GiftOfHeaven.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "AbilitySystemComponent.h"
#include "GAS/CAbilitySystemStatics.h"

UGA_GiftOfHeaven::UGA_GiftOfHeaven()
{
	SetAssetTags(FGameplayTagContainer(UCAbilitySystemStatics::GetPassiveAbilityTypeTag()));

	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
}

void UGA_GiftOfHeaven::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!HasAuthorityOrPredictionKey(ActorInfo, &ActivationInfo))
	{
		K2_EndAbility();
		return;
	}

	if (LifeStealBuffHandle.IsValid())
		return;

	int32 AbilityLevel = GetAbilityLevel(Handle, ActorInfo);

	if (AbilityLevel <= 0)
		return;

	LifeStealBuffHandle = BP_ApplyGameplayEffectToOwner(LifeStealBuffEffect, AbilityLevel);
}

void UGA_GiftOfHeaven::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (ASC && LifeStealBuffHandle.IsValid())
	{
		ASC->RemoveActiveGameplayEffect(LifeStealBuffHandle);
		LifeStealBuffHandle.Invalidate();
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_GiftOfHeaven::OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnAvatarSet(ActorInfo, Spec);

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (ASC)
	{
		ASC->AbilitySpecDirtiedCallbacks.AddUObject(this, &UGA_GiftOfHeaven::OnAbilitySpecDirtied);
	}
}

void UGA_GiftOfHeaven::OnAbilitySpecDirtied(const FGameplayAbilitySpec& Spec)
{
	if (Spec.Ability != GetClass()->GetDefaultObject<UGameplayAbility>() && Spec.Ability != this)
		return;

	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (!ASC)
		return;

	if (Spec.Level >= 1)
	{
		if (LifeStealBuffHandle.IsValid())
		{
			ASC->RemoveActiveGameplayEffect(LifeStealBuffHandle);
			LifeStealBuffHandle.Invalidate();
		}
		LifeStealBuffHandle = BP_ApplyGameplayEffectToOwner(LifeStealBuffEffect, Spec.Level);
	}
}
