// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/GA_Freeze.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitTargetData.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GAS/CAbilitySystemStatics.h"
#include "GAS/TargetActor_GroundPick.h"

UGA_Freeze::UGA_Freeze()
{
	ActivationOwnedTags.AddTag(UCAbilitySystemStatics::GetSkillAimStatusTag());
}

void UGA_Freeze::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!HasAuthorityOrPredictionKey(CurrentActorInfo, &CurrentActivationInfo))
		return;

	UAbilityTask_PlayMontageAndWait* PlayTargetingMontage = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, TargetingMontage);
	PlayTargetingMontage->OnBlendOut.AddDynamic(this, &UGA_Freeze::K2_EndAbility);
	PlayTargetingMontage->OnInterrupted.AddDynamic(this, &UGA_Freeze::K2_EndAbility);
	PlayTargetingMontage->OnCancelled.AddDynamic(this, &UGA_Freeze::K2_EndAbility);
	PlayTargetingMontage->OnCompleted.AddDynamic(this, &UGA_Freeze::K2_EndAbility);
	PlayTargetingMontage->ReadyForActivation();

	UAbilityTask_WaitTargetData* WaitTargetData = UAbilityTask_WaitTargetData::WaitTargetData(this, NAME_None, EGameplayTargetingConfirmation::UserConfirmed, TargetActorClass);
	WaitTargetData->ValidData.AddDynamic(this, &UGA_Freeze::TargetReceived);
	WaitTargetData->Cancelled.AddDynamic(this, &UGA_Freeze::TargetingCancelled);
	WaitTargetData->ReadyForActivation();

	AGameplayAbilityTargetActor* TargetActor;
	WaitTargetData->BeginSpawningActor(this, TargetActorClass, TargetActor);
	ATargetActor_GroundPick* TargetActor_GroundPick = Cast<ATargetActor_GroundPick>(TargetActor);
	if (TargetActor_GroundPick)
	{
		TargetActor_GroundPick->SetShouldDrawDebug(ShouldDrawDebug());
		TargetActor_GroundPick->SetTargetAreaRadius(TargetingRadius);
		TargetActor_GroundPick->SetTargetTraceRange(TargetingRange);
	}
	WaitTargetData->FinishSpawningActor(this, TargetActor);
}

FGameplayTag UGA_Freeze::GetFreezeCastTag()
{
	return FGameplayTag::RequestGameplayTag("Ability.Freeze.Cast");
}

void UGA_Freeze::TargetReceived(const FGameplayAbilityTargetDataHandle& TargetDataHandle)
{
	if (!K2_CommitAbility())
	{
		K2_EndAbility();
		return;
	}

	PlayMontageLocally(CastingMontage);

	if (K2_HasAuthority())
	{
		int32 AbilityLevel = GetAbilityLevel(CurrentSpecHandle, CurrentActorInfo);
		float BaseDamage = GetLevelIndexedDamage(BaseDamagePerLevel, AbilityLevel);
		FGameplayEffectSpecHandle DamageSpec = MakeScaledDamageSpec(DamageEffect, BaseDamage, ScalingRatio, AbilityLevel);
		ApplyGameplayEffectSpecToTarget(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, DamageSpec, TargetDataHandle);
	}

	FGameplayCueParameters FreezeCueParameters;
	FreezeCueParameters.Location = UAbilitySystemBlueprintLibrary::GetHitResultFromTargetData(TargetDataHandle, 1).ImpactPoint;
	FreezeCueParameters.RawMagnitude = TargetingRadius;

	GetAbilitySystemComponentFromActorInfo()->ExecuteGameplayCue(FreezeTargetingGameplayCueTag, FreezeCueParameters);


	K2_EndAbility();
}
	
void UGA_Freeze::TargetingCancelled(const FGameplayAbilityTargetDataHandle& TargetDataHandle)
{
	K2_EndAbility();
}
