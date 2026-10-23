// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/GA_Teleport.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitTargetData.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GAS/CAbilitySystemStatics.h"
#include "GAS/TargetActor_GroundPick.h"

UGA_Teleport::UGA_Teleport()
{
	ActivationOwnedTags.AddTag(UCAbilitySystemStatics::GetSkillAimStatusTag());
}

void UGA_Teleport::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!HasAuthorityOrPredictionKey(ActorInfo, &ActivationInfo))
	{
		K2_EndAbility();
		return;
	}

	UAbilityTask_PlayMontageAndWait* PlayTargetingMontage = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, TargetingMontage);
	PlayTargetingMontage->ReadyForActivation();

	UAbilityTask_WaitTargetData* WaitBlinkLocationTargetData = UAbilityTask_WaitTargetData::WaitTargetData(this, NAME_None, EGameplayTargetingConfirmation::UserConfirmed, GroundPickTargetActorClass);
	WaitBlinkLocationTargetData->ValidData.AddDynamic(this, &UGA_Teleport::GroundPickTargetReceived);
	WaitBlinkLocationTargetData->Cancelled.AddDynamic(this, &UGA_Teleport::GroundPickCancelled);
	WaitBlinkLocationTargetData->ReadyForActivation();

	AGameplayAbilityTargetActor* TargetActor;
	WaitBlinkLocationTargetData->BeginSpawningActor(this, GroundPickTargetActorClass, TargetActor);
	ATargetActor_GroundPick* GroundPickTargetActor = Cast<ATargetActor_GroundPick>(TargetActor);
	if (GroundPickTargetActor)
	{
		GroundPickTargetActor->SetShouldDrawDebug(ShouldDrawDebug());
		GroundPickTargetActor->SetTargetAreaRadius(TeleportAreaRadius);
		GroundPickTargetActor->SetTargetTraceRange(TeleportCastRange);
	}

	WaitBlinkLocationTargetData->FinishSpawningActor(this, TargetActor);
}

FGameplayTag UGA_Teleport::GetTeleportTag()
{
	return FGameplayTag::RequestGameplayTag("Ability.Blink.Teleport");
}

void UGA_Teleport::GroundPickTargetReceived(const FGameplayAbilityTargetDataHandle& TargetDataHandle)
{
	if (!K2_CommitAbility())
	{
		K2_EndAbility();
		return;
	}

	TeleportTargetDataHandle = TargetDataHandle;

	if (HasAuthorityOrPredictionKey(CurrentActorInfo, &CurrentActivationInfo))
	{
		UAbilityTask_PlayMontageAndWait* PlayTeleportMontage = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, TeleportMontage);
		PlayTeleportMontage->OnBlendOut.AddDynamic(this, &UGA_Teleport::K2_EndAbility);
		PlayTeleportMontage->OnInterrupted.AddDynamic(this, &UGA_Teleport::K2_EndAbility);
		PlayTeleportMontage->OnCancelled.AddDynamic(this, &UGA_Teleport::K2_EndAbility);
		PlayTeleportMontage->OnCompleted.AddDynamic(this, &UGA_Teleport::K2_EndAbility);
		PlayTeleportMontage->ReadyForActivation();

		if (K2_HasAuthority())
		{
			UAbilityTask_WaitGameplayEvent* WaitTeleportTimePoint = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, GetTeleportTag());
			WaitTeleportTimePoint->EventReceived.AddDynamic(this, &UGA_Teleport::Teleport);
			WaitTeleportTimePoint->ReadyForActivation();
		}
	}
}

void UGA_Teleport::GroundPickCancelled(const FGameplayAbilityTargetDataHandle& TargetDataHandle)
{
	K2_EndAbility();
}

void UGA_Teleport::Teleport(FGameplayEventData Payload)
{
	if (K2_HasAuthority())
	{
		FHitResult PickedLocationHitResult = UAbilitySystemBlueprintLibrary::GetHitResultFromTargetData(TeleportTargetDataHandle, 1);
		FVector PickedTeleportLocation = PickedLocationHitResult.ImpactPoint;

		GetAvatarActorFromActorInfo()->SetActorLocation(PickedTeleportLocation);

		int32 AbilityLevel = GetAbilityLevel(CurrentSpecHandle, CurrentActorInfo);
		float BaseDamage = GetLevelIndexedDamage(BaseDamagePerLevel, AbilityLevel);
		FGameplayEffectSpecHandle DamageSpec = MakeScaledDamageSpec(DamageEffect, BaseDamage, ScalingRatio, AbilityLevel);
		ApplyGameplayEffectSpecToTarget(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, DamageSpec, TeleportTargetDataHandle);

		PushTargetsFromOwnerLocation(TeleportTargetDataHandle, TeleportLandTargetPushSpeed, false);
	}
}
