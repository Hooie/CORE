// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/GA_Tornado.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitCancel.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "GAS/CAbilitySystemStatics.h"
#include "Widgets/GameplayWidget.h"

UGA_Tornado::UGA_Tornado()
{
	SetAssetTags(FGameplayTagContainer(FGameplayTag::RequestGameplayTag("Ability.Type.Ultimate")));
}

void UGA_Tornado::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!K2_CommitAbility())
	{
		K2_EndAbility();
		return;
	}

	if (HasAuthorityOrPredictionKey(CurrentActorInfo, &CurrentActivationInfo))
	{
		UAbilityTask_PlayMontageAndWait* PlayTornadoMontage = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, TornadoMontage);
		PlayTornadoMontage->OnBlendOut.AddDynamic(this, &UGA_Tornado::K2_EndAbility);
		PlayTornadoMontage->OnInterrupted.AddDynamic(this, &UGA_Tornado::K2_EndAbility);
		PlayTornadoMontage->OnCancelled.AddDynamic(this, &UGA_Tornado::K2_EndAbility);
		PlayTornadoMontage->OnCompleted.AddDynamic(this, &UGA_Tornado::K2_EndAbility);
		PlayTornadoMontage->ReadyForActivation();

		if (K2_HasAuthority())
		{
			UAbilityTask_WaitGameplayEvent* WaitDamageEvent = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, UCAbilitySystemStatics::GetGenericDamageTag(), nullptr, false);
			WaitDamageEvent->EventReceived.AddDynamic(this, &UGA_Tornado::TornadoDamageEventReceived);
			WaitDamageEvent->ReadyForActivation();
		}

		UAbilityTask_WaitCancel* WaitCancel = UAbilityTask_WaitCancel::WaitCancel(this);
		WaitCancel->OnCancel.AddDynamic(this, &UGA_Tornado::K2_EndAbility);
		WaitCancel->ReadyForActivation();

		UAbilityTask_WaitDelay* WaitDelay = UAbilityTask_WaitDelay::WaitDelay(this, TornadoDuration);
		WaitDelay->OnFinish.AddDynamic(this, &UGA_Tornado::K2_EndAbility);
		WaitDelay->ReadyForActivation();

		if ( UGameplayWidget* Widget = GetOwningGameplayWidget() )
		{
			Widget->ShowHoldProgress(TornadoDuration, true);
		}
	}
}

void UGA_Tornado::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if ( UGameplayWidget* Widget = GetOwningGameplayWidget() )
	{
		Widget->HideHoldProgress();
	}
	
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_Tornado::TornadoDamageEventReceived(FGameplayEventData Payload)
{
	if (K2_HasAuthority())
	{
		FGameplayAbilityTargetDataHandle TargetDataHandle = Payload.TargetData;

		int32 AbilityLevel = GetAbilityLevel(CurrentSpecHandle, CurrentActorInfo);
		float BaseDamage = GetLevelIndexedDamage(BaseDamagePerLevel, AbilityLevel);
		FGameplayEffectSpecHandle DamageSpec = MakeScaledDamageSpec(DamageEffect, BaseDamage, ScalingRatio, AbilityLevel);

		ApplyGameplayEffectSpecToTarget(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, DamageSpec, TargetDataHandle);
		PushTargetsFromOwnerLocation(TargetDataHandle, HitPushSpeed, false);
	}
}
