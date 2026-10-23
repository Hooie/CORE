// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/GA_Reload.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "GAS/CAbilitySystemStatics.h"
#include "Player/AmmoComponent.h"
#include "Widgets/GameplayWidget.h"

UGA_Reload::UGA_Reload()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	FGameplayTagContainer OwnedTags;
	OwnedTags.AddTag(UCAbilitySystemStatics::GetReloadingStatusTag());
	SetAssetTags(OwnedTags);

	ActivationOwnedTags.AddTag(UCAbilitySystemStatics::GetReloadingStatusTag());
}

bool UGA_Reload::CanActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayTagContainer* SourceTags, const FGameplayTagContainer* TargetTags, FGameplayTagContainer* OptionalRelevantTags) const
{
	if ( !Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags) )
		return false;

	if ( ActorInfo && ActorInfo->IsNetAuthority() )
	{
		if ( AActor* Avatar = ActorInfo->AvatarActor.Get() )
		{
			if ( UAmmoComponent* Ammo = Avatar->FindComponentByClass<UAmmoComponent>() )
			{
				if ( Ammo->GetCurrentAmmo() >= Ammo->GetMaxAmmo() )
				{
					return false;
				}
			}
		}
	}

	return true;
}

void UGA_Reload::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if ( !K2_CommitAbility() || !ReloadMontage)
	{
		K2_EndAbility();
		return;
	}

	if ( K2_HasAuthority() )
	{
		UAbilityTask_WaitGameplayEvent* WaitRefillEvent = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, GetReloadRefillTag());
		WaitRefillEvent->EventReceived.AddDynamic(this, &UGA_Reload::OnReloadRefillEvent);
		WaitRefillEvent->ReadyForActivation();
	}

	if ( K2_HasAuthority() || IsLocallyControlled() )
	{
		UAbilityTask_PlayMontageAndWait* PlayReloadMontage = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, ReloadMontage);
		PlayReloadMontage->OnBlendOut.AddDynamic(this, &UGA_Reload::OnReloadMontageFinished);
		PlayReloadMontage->OnInterrupted.AddDynamic(this, &UGA_Reload::OnReloadMontageFinished);
		PlayReloadMontage->OnCancelled.AddDynamic(this, &UGA_Reload::OnReloadMontageFinished);
		PlayReloadMontage->ReadyForActivation();
	}

	if ( UGameplayWidget* Widget = GetOwningGameplayWidget() )
	{
		Widget->ShowHoldProgress(2.1f, true);
	}
}

void UGA_Reload::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if ( UGameplayWidget* Widget = GetOwningGameplayWidget() )
	{
		Widget->HideHoldProgress();
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

FGameplayTag UGA_Reload::GetReloadRefillTag()
{
	return FGameplayTag::RequestGameplayTag("Ability.Reload.Refill");
}

void UGA_Reload::OnReloadMontageFinished()
{
	K2_EndAbility();
}

void UGA_Reload::OnReloadRefillEvent(FGameplayEventData Payload)
{
	if ( K2_HasAuthority() )
	{
		if ( AActor* Avatar = GetAvatarActorFromActorInfo() )
		{
			if ( UAmmoComponent* AmmoComponent = Avatar->FindComponentByClass<UAmmoComponent>() )
			{
				AmmoComponent->Reload();
			}
		}
	}
}
