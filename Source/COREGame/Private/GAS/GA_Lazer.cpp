// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/GA_Lazer.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitCancel.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_WaitTargetData.h"
#include "GAS/CAttributeSet.h"
#include "GAS/TargetActor_Line.h"


void UGA_Lazer::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!K2_CommitAbility() || !LazerMontage)
	{
		K2_EndAbility();
		return;
	}

	if (HasAuthorityOrPredictionKey(ActorInfo, &ActivationInfo))
	{
		UAbilityTask_PlayMontageAndWait* PlayLazerMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, LazerMontage);
		PlayLazerMontageTask->OnBlendOut.AddDynamic(this, &UGA_Lazer::K2_EndAbility);
		PlayLazerMontageTask->OnCancelled.AddDynamic(this, &UGA_Lazer::K2_EndAbility);
		PlayLazerMontageTask->OnInterrupted.AddDynamic(this, &UGA_Lazer::K2_EndAbility);
		PlayLazerMontageTask->OnCompleted.AddDynamic(this, &UGA_Lazer::K2_EndAbility);
		PlayLazerMontageTask->ReadyForActivation();

		UAbilityTask_WaitGameplayEvent* WaitShootEvent = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, GetShootTag());
		WaitShootEvent->EventReceived.AddDynamic(this, &UGA_Lazer::ShootLazer);
		WaitShootEvent->ReadyForActivation();

		UAbilityTask_WaitCancel* WaitCancel = UAbilityTask_WaitCancel::WaitCancel(this);
		WaitCancel->OnCancel.AddDynamic(this, &UGA_Lazer::K2_EndAbility);
		WaitCancel->ReadyForActivation();
	}
}

void UGA_Lazer::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	UAbilitySystemComponent* OwnerASC = GetAbilitySystemComponentFromActorInfo();

	if (OwnerASC && OnGoingConsumptionEffectHandle.IsValid())
	{
		OwnerASC->RemoveActiveGameplayEffect(OnGoingConsumptionEffectHandle);
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

FGameplayTag UGA_Lazer::GetShootTag()
{
	return FGameplayTag::RequestGameplayTag("Ability.Lazer.Shoot");
}

void UGA_Lazer::ShootLazer(FGameplayEventData Payload)
{
	if (K2_HasAuthority())
	{
		OnGoingConsumptionEffectHandle = BP_ApplyGameplayEffectToOwner(OnGoingConsumptionEffect, GetAbilityLevel(CurrentSpecHandle, CurrentActorInfo));
		UAbilitySystemComponent* OwnerASC = GetAbilitySystemComponentFromActorInfo();

		if (OwnerASC)
		{
			OwnerASC->GetGameplayAttributeValueChangeDelegate(UCAttributeSet::GetManaAttribute()).AddUObject(this, &UGA_Lazer::ManaUpdated);
		}
	}

	UAbilityTask_WaitTargetData* WaitDamageTargetTask = UAbilityTask_WaitTargetData::WaitTargetData(this, NAME_None, EGameplayTargetingConfirmation::CustomMulti, LazerTargetActorClass);
	WaitDamageTargetTask->ValidData.AddDynamic(this, &UGA_Lazer::TargetReceived);
	WaitDamageTargetTask->ReadyForActivation();

	AGameplayAbilityTargetActor* TargetActor;
	WaitDamageTargetTask->BeginSpawningActor(this, LazerTargetActorClass, TargetActor);

	ATargetActor_Line* LineTargetActor = Cast<ATargetActor_Line>(TargetActor);
	if (LineTargetActor)
	{
		LineTargetActor->ConfigureTargetSetting(TargetRange, DetectionCylinderRadius, TargetingInterval, GetOwnerTeamId(), ShouldDrawDebug());
	}
	WaitDamageTargetTask->FinishSpawningActor(this, TargetActor);

	if(LineTargetActor)
		LineTargetActor->AttachToComponent(GetOwningComponentFromActorInfo(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, TargetActorAttachSocketName);
}

void UGA_Lazer::ManaUpdated(const FOnAttributeChangeData& ChangeData)
{
	UAbilitySystemComponent* OwnerASC = GetAbilitySystemComponentFromActorInfo();

	if (OwnerASC && !OwnerASC->CanApplyAttributeModifiers(OnGoingConsumptionEffect.GetDefaultObject(), GetAbilityLevel(CurrentSpecHandle, CurrentActorInfo), MakeEffectContext(CurrentSpecHandle, CurrentActorInfo)))
	{
		K2_EndAbility();
	}
}

void UGA_Lazer::TargetReceived(const FGameplayAbilityTargetDataHandle& TargetDataHandle)
{
	if (K2_HasAuthority())
	{
		int32 AbilityLevel = GetAbilityLevel(CurrentSpecHandle, CurrentActorInfo);
		float BaseDamage = GetLevelIndexedDamage(BaseDamagePerLevel, AbilityLevel);
		FGameplayEffectSpecHandle DamageSpec = MakeScaledDamageSpec(HitDamageEffect, BaseDamage, ScalingRatio, AbilityLevel);
		ApplyGameplayEffectSpecToTarget(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, DamageSpec, TargetDataHandle);
	}
	PushTargets(TargetDataHandle, GetAvatarActorFromActorInfo()->GetActorForwardVector()* HitPushSpeed);
}
