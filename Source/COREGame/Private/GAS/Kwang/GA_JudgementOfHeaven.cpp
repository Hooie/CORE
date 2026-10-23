// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Kwang/GA_JudgementOfHeaven.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitTargetData.h"
#include "Abilities/Tasks/AbilityTask_WaitInputPress.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Character/CKwangCharacter.h"
#include "GAS/Kwang/TA_Sword.h"
#include "GAS/TargetActor_GroundPick.h"
#include "GAS/CAbilitySystemStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Widgets/GameplayWidget.h"

UGA_JudgementOfHeaven::UGA_JudgementOfHeaven()
{
	SetAssetTags(FGameplayTagContainer(FGameplayTag::RequestGameplayTag("Ability.Kwang.Judgement")));

	BlockAbilitiesWithTag.AddTag(UCAbilitySystemStatics::GetBasicAttackAbilityTag());
}

void UGA_JudgementOfHeaven::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!HasAuthorityOrPredictionKey(ActorInfo, &ActivationInfo) || !K2_CheckAbilityCost() )
	{
		K2_EndAbility();
		return;
	}

	PlayCastSwordMontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, TargetingMontage);
	PlayCastSwordMontageTask->OnBlendOut.AddDynamic(this, &UGA_JudgementOfHeaven::K2_EndAbility);
	PlayCastSwordMontageTask->OnInterrupted.AddDynamic(this, &UGA_JudgementOfHeaven::K2_EndAbility);
	PlayCastSwordMontageTask->OnCancelled.AddDynamic(this, &UGA_JudgementOfHeaven::K2_EndAbility);
	PlayCastSwordMontageTask->ReadyForActivation();

	UAbilityTask_WaitTargetData* WaitPlacementTask = UAbilityTask_WaitTargetData::WaitTargetData(this, NAME_None, EGameplayTargetingConfirmation::UserConfirmed, TargetActorClass);
	WaitPlacementTask->ValidData.AddDynamic(this, &UGA_JudgementOfHeaven::PlaceSword);
	WaitPlacementTask->Cancelled.AddDynamic(this, &UGA_JudgementOfHeaven::PlacementCancelled);
	WaitPlacementTask->ReadyForActivation();

	AGameplayAbilityTargetActor* TargetActor;
	WaitPlacementTask->BeginSpawningActor(this, TargetActorClass, TargetActor);
	ATargetActor_GroundPick* GroundPickTargetActor = Cast<ATargetActor_GroundPick>(TargetActor);
	if (GroundPickTargetActor)
	{
		GroundPickTargetActor->SetShouldDrawDebug(ShouldDrawDebug());
		GroundPickTargetActor->SetTargetAreaRadius(TargetAreaRadius);
		GroundPickTargetActor->SetTargetTraceRange(TargetTraceRange);
	}
	WaitPlacementTask->FinishSpawningActor(this, TargetActor);

	AddSkillAimEffect();
}

void UGA_JudgementOfHeaven::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	RemoveSkillAimEffect();
	RemoveNoWeaponEffect();

	if ( UGameplayWidget* Widget = GetOwningGameplayWidget() )
	{
		Widget->HideHoldProgress();
	}

	ACKwangCharacter* Kwang = Cast<ACKwangCharacter>(GetAvatarActorFromActorInfo());
	if ( Kwang )
	{
		Kwang->SetSpawnedSword(nullptr);
		Kwang->Multicast_ToggleSwordVisibility(true);
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

FGameplayTag UGA_JudgementOfHeaven::GetFinalTag()
{
	return FGameplayTag::RequestGameplayTag("Ability.Kwang.Judgement.Final");
}

FGameplayTag UGA_JudgementOfHeaven::GetThrowTag()
{
	return FGameplayTag::RequestGameplayTag("Ability.Kwang.Judgement.Throw");
}

void UGA_JudgementOfHeaven::OnRetriggerInputPressed(float TimeWaited)
{
	if (SwordTargetActor)
	{
		SwordTargetActor->EarlyTerminate();
	}
}

void UGA_JudgementOfHeaven::PlaceSword(const FGameplayAbilityTargetDataHandle& TargetDataHandle)
{
	if (!K2_CommitAbility())
	{
		K2_EndAbility();
		return;
	}

	RemoveSkillAimEffect();
	AddNoWeaponEffect();

	if (PlayCastSwordMontageTask)
	{
		PlayCastSwordMontageTask->OnBlendOut.RemoveAll(this);
		PlayCastSwordMontageTask->OnCancelled.RemoveAll(this);
		PlayCastSwordMontageTask->OnInterrupted.RemoveAll(this);
		PlayCastSwordMontageTask->OnCompleted.RemoveAll(this);
	}

	if (HasAuthorityOrPredictionKey(CurrentActorInfo, &CurrentActivationInfo))
	{
		UAbilityTask_PlayMontageAndWait* PlayThrowSwordMontage = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, ThrowingSwordMontage);
		PlayThrowSwordMontage->OnCancelled.AddDynamic(this, &UGA_JudgementOfHeaven::K2_EndAbility);
		//PlayThrowSwordMontage->OnInterrupted.AddDynamic(this, &UGA_JudgementOfHeaven::K2_EndAbility);
		//PlayThrowSwordMontage->OnBlendOut.AddDynamic(this, &UGA_JudgementOfHeaven::K2_EndAbility);
		//PlayThrowSwordMontage->OnCompleted.AddDynamic(this, &UGA_JudgementOfHeaven::K2_EndAbility);
		PlayThrowSwordMontage->ReadyForActivation();

		UAbilityTask_WaitGameplayEvent* WaitThrowEvent = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, GetThrowTag());
		WaitThrowEvent->EventReceived.AddDynamic(this, &UGA_JudgementOfHeaven::ThrowTagReceived);
		WaitThrowEvent->ReadyForActivation();
	}

	AfterThrowingSwordTask = UAbilityTask_WaitTargetData::WaitTargetData(this, NAME_None, EGameplayTargetingConfirmation::Custom, SwordTargetActorClass);
	AfterThrowingSwordTask->ValidData.AddDynamic(this, &UGA_JudgementOfHeaven::FinalTargetReceived);
	AfterThrowingSwordTask->Cancelled.AddDynamic(this, &UGA_JudgementOfHeaven::FinalTargetReceived);
	AfterThrowingSwordTask->ReadyForActivation();

	int32 LastIndex = TargetDataHandle.Data.Num() - 1;
	FVector SwordSpawnLocation = UAbilitySystemBlueprintLibrary::GetHitResultFromTargetData(TargetDataHandle, LastIndex).ImpactPoint;

	AGameplayAbilityTargetActor* TargetActor;
	AfterThrowingSwordTask->BeginSpawningActor(this, SwordTargetActorClass, TargetActor);
	SwordTargetActor = Cast<ATA_Sword>(TargetActor);
	if (SwordTargetActor)
	{
		SwordTargetActor->ConfigureSword(TargetAreaRadius, SwordBindInterval, SwordDuration, GetOwnerTeamId());
	}

	AfterThrowingSwordTask->FinishSpawningActor(this, TargetActor);

	if (SwordTargetActor)
	{
		SwordTargetActor->SetSwordLocation(SwordSpawnLocation);
		//SwordTargetActor->ReplicatedLocation = SwordSpawnLocation;
		SwordTargetActor->BeginBindTimer();
		SwordTargetActor->OnFuryDisabled.AddUObject(this, &UGA_JudgementOfHeaven::DisableFury);

		WaitRetriggerTask = UAbilityTask_WaitInputPress::WaitInputPress(this);
		WaitRetriggerTask->OnPress.AddDynamic(this, &UGA_JudgementOfHeaven::OnRetriggerInputPressed);
		WaitRetriggerTask->ReadyForActivation();

		ACKwangCharacter* Kwang = Cast<ACKwangCharacter>(GetAvatarActorFromActorInfo());
		if ( Kwang )
		{
			Kwang->SetSpawnedSword(SwordTargetActor);
		}

		if ( UGameplayWidget* Widget = GetOwningGameplayWidget() )
		{
			Widget->ShowHoldProgress(SwordDuration, true);
		}
	}
	
	TArray<FHitResult> HitResults = GetHitResultFromSweepLocationTargetData(TargetDataHandle, TargetAreaRadius);

	int32 AbilityLevel = GetAbilityLevel(CurrentSpecHandle, CurrentActorInfo);
	float BaseDamage = GetLevelIndexedDamage(SwordBaseDamagePerLevel, AbilityLevel);

	FGameplayEffectSpecHandle DamageSpec = MakeScaledDamageSpec(SwordDamageEffect, BaseDamage, SwordScalingRatio, AbilityLevel);

	for ( FHitResult& HitResult : HitResults )
	{
		ApplyGameplayEffectToHitResultActor(HitResult, DamageSpec);
	}
}

void UGA_JudgementOfHeaven::PlacementCancelled(const FGameplayAbilityTargetDataHandle& TargetDataHandle)
{
	K2_EndAbility();
}

void UGA_JudgementOfHeaven::FinalTargetReceived(const FGameplayAbilityTargetDataHandle& TargetDataHandle)
{
	UE_LOG(LogTemp, Warning, TEXT("JudgementOfHeaven: FinalTargetReceived"));

	if ( UGameplayWidget* Widget = GetOwningGameplayWidget() )
	{
		Widget->HideHoldProgress();
	}

	if (K2_HasAuthority())
		RemoveNoWeaponEffect();

	if (HasAuthorityOrPredictionKey(CurrentActorInfo, &CurrentActivationInfo))
	{
		UAbilityTask_PlayMontageAndWait* PlayFinalMontage = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, FinalMontage);
		//PlayFinalMontage->OnCancelled.AddDynamic(this, &UGA_JudgementOfHeaven::K2_EndAbility);
		//PlayFinalMontage->OnInterrupted.AddDynamic(this, &UGA_JudgementOfHeaven::K2_EndAbility);
		PlayFinalMontage->OnBlendOut.AddDynamic(this, &UGA_JudgementOfHeaven::K2_EndAbility);
		PlayFinalMontage->OnCompleted.AddDynamic(this, &UGA_JudgementOfHeaven::K2_EndAbility);
		PlayFinalMontage->ReadyForActivation();

		UAbilityTask_WaitGameplayEvent* WaitFinalEvent = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, GetFinalTag());
		WaitFinalEvent->EventReceived.AddDynamic(this, &UGA_JudgementOfHeaven::FinalTagReceived);
		WaitFinalEvent->ReadyForActivation();
	}
	else
	{
		PlayMontageLocally(FinalMontage);
	}
}

void UGA_JudgementOfHeaven::AddSkillAimEffect()
{
	SkillAimEffectHandle = BP_ApplyGameplayEffectToOwner(SkillAimEffect);
}

void UGA_JudgementOfHeaven::RemoveSkillAimEffect()
{
	if ( SkillAimEffectHandle.IsValid())
	{
		BP_RemoveGameplayEffectFromOwnerWithHandle(SkillAimEffectHandle);
	}
}

void UGA_JudgementOfHeaven::AddNoWeaponEffect()
{
	NoWeaponEffectHandle = BP_ApplyGameplayEffectToOwner(NoWeaponEffect);
}

void UGA_JudgementOfHeaven::RemoveNoWeaponEffect()
{
	if (NoWeaponEffectHandle.IsValid())
	{
		BP_RemoveGameplayEffectFromOwnerWithHandle(NoWeaponEffectHandle);
	}
}

void UGA_JudgementOfHeaven::SetSwordVisibility(bool bVisible)
{
	if (!K2_HasAuthority())
		return;

	ACKwangCharacter* Kwang = Cast<ACKwangCharacter>(GetAvatarActorFromActorInfo());
	if (Kwang)
	{
		Kwang->Multicast_ToggleSwordVisibility(bVisible);
	}
}

void UGA_JudgementOfHeaven::FinalTagReceived(FGameplayEventData Payload)
{
	SetSwordVisibility(true);
}

void UGA_JudgementOfHeaven::ThrowTagReceived(FGameplayEventData Payload)
{
	SetSwordVisibility(false);
}

void UGA_JudgementOfHeaven::DisableFury()
{
	BP_ApplyGameplayEffectToOwner(DisableFuryEffect);
}