// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/GA_GroundBlast.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GAS/CAbilitySystemStatics.h"
#include "GAS/TargetActor_GroundPick.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitTargetData.h"

UGA_GroundBlast::UGA_GroundBlast()
{
	ActivationOwnedTags.AddTag(UCAbilitySystemStatics::GetSkillAimStatusTag());
	BlockAbilitiesWithTag.AddTag(UCAbilitySystemStatics::GetBasicAttackAbilityTag());
}

void UGA_GroundBlast::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!HasAuthorityOrPredictionKey(CurrentActorInfo, &CurrentActivationInfo))
		return;

	UAbilityTask_PlayMontageAndWait* PlayGroundBlastAnimTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, TargetMontage);
	PlayGroundBlastAnimTask->OnBlendOut.AddDynamic(this, &UGA_GroundBlast::K2_EndAbility);
	PlayGroundBlastAnimTask->OnInterrupted.AddDynamic(this, &UGA_GroundBlast::K2_EndAbility);
	PlayGroundBlastAnimTask->OnCancelled.AddDynamic(this, &UGA_GroundBlast::K2_EndAbility);
	PlayGroundBlastAnimTask->OnCompleted.AddDynamic(this, &UGA_GroundBlast::K2_EndAbility);
	PlayGroundBlastAnimTask->ReadyForActivation();

	UAbilityTask_WaitTargetData* WaitTargetDataTask = UAbilityTask_WaitTargetData::WaitTargetData(this, NAME_None, EGameplayTargetingConfirmation::UserConfirmed, TargetActorClass);
	WaitTargetDataTask->ValidData.AddDynamic(this, &UGA_GroundBlast::TargetConfirmed);
	WaitTargetDataTask->Cancelled.AddDynamic(this, &UGA_GroundBlast::TargetCancelled);
	WaitTargetDataTask->ReadyForActivation();

	AGameplayAbilityTargetActor* TargetActor;
	WaitTargetDataTask->BeginSpawningActor(this, TargetActorClass, TargetActor);

	ATargetActor_GroundPick* GroundPickActor = Cast<ATargetActor_GroundPick>(TargetActor);
	if (GroundPickActor)
	{
		GroundPickActor->SetShouldDrawDebug(ShouldDrawDebug());
		GroundPickActor->SetTargetAreaRadius(TargetAreaRadius);
		GroundPickActor->SetTargetTraceRange(TargetTraceRange);
	}

	WaitTargetDataTask->FinishSpawningActor(this, TargetActor);
}

void UGA_GroundBlast::TargetConfirmed(const FGameplayAbilityTargetDataHandle& TargetDataHandle)
{
	if (!K2_CommitAbility())
	{
		K2_EndAbility();
		return;
	}

	if (K2_HasAuthority())
	{
		int32 AbilityLevel = GetAbilityLevel(CurrentSpecHandle, CurrentActorInfo);
		float BaseDamage = GetLevelIndexedDamage(DamageEffectDef.BaseDamagePerLevel, AbilityLevel);
		FGameplayEffectSpecHandle BlastSpec = MakeScaledDamageSpec(DamageEffectDef.DamageEffect, BaseDamage, DamageEffectDef.ScalingRatio, AbilityLevel);

		ApplyGameplayEffectSpecToTarget(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, BlastSpec, TargetDataHandle);
		PushTargets(TargetDataHandle, DamageEffectDef.PushVelocity);		
	}

	FGameplayCueParameters BlastingGameplayCueParams;
	BlastingGameplayCueParams.Location = UAbilitySystemBlueprintLibrary::GetHitResultFromTargetData(TargetDataHandle, 1).ImpactPoint;
	BlastingGameplayCueParams.RawMagnitude = TargetAreaRadius;

	GetAbilitySystemComponentFromActorInfo()->ExecuteGameplayCue(BlastGameplayCueTag, BlastingGameplayCueParams);
	GetAbilitySystemComponentFromActorInfo()->ExecuteGameplayCue(UCAbilitySystemStatics::GetCameraShakeGameplayCueTag(), BlastingGameplayCueParams);
	
	UAnimInstance* OwnerAnimInst = GetOwnerAnimInstance();
	if (OwnerAnimInst)
	{
		OwnerAnimInst->Montage_Play(CastMontage);
	}

	K2_EndAbility();
}

void UGA_GroundBlast::TargetCancelled(const FGameplayAbilityTargetDataHandle& TargetDataHandle)
{
	UE_LOG(LogTemp, Warning, TEXT("Target Cancelled"));
	K2_EndAbility();
}
