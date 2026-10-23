// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Murdock/GA_Murdock_StaticTrap.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Abilities/Tasks/AbilityTask_WaitTargetData.h"
#include "GAS/Murdock/TA_StaticTrap.h"
#include "GAS/TargetActor_GroundPick.h"
#include "Kismet/GameplayStatics.h"

UGA_Murdock_StaticTrap::UGA_Murdock_StaticTrap()
{
	FGameplayTagContainer OwnedTags;
	OwnedTags.AddTag(FGameplayTag::RequestGameplayTag("Ability.Murdock.Trap"));
	SetAssetTags(OwnedTags);
}

void UGA_Murdock_StaticTrap::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if ( !HasAuthorityOrPredictionKey(ActorInfo, &ActivationInfo) )
	{
		K2_EndAbility();
		return;
	}

	if ( !TargetingMontage || !ThrowingMontage || !K2_CheckAbilityCost() )
	{
		K2_EndAbility();
		return;
	}

	PlayTargetingMontage = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, TargetingMontage);
	PlayTargetingMontage->OnInterrupted.AddDynamic(this, &UGA_Murdock_StaticTrap::K2_EndAbility);
	PlayTargetingMontage->OnCancelled.AddDynamic(this, &UGA_Murdock_StaticTrap::K2_EndAbility);
	PlayTargetingMontage->ReadyForActivation();

	UAbilityTask_WaitTargetData* WaitPlaceTrap = UAbilityTask_WaitTargetData::WaitTargetData(this, NAME_None, EGameplayTargetingConfirmation::UserConfirmed, TargetActorClass);
	WaitPlaceTrap->ValidData.AddDynamic(this, &UGA_Murdock_StaticTrap::PlaceTrap);
	WaitPlaceTrap->Cancelled.AddDynamic(this, &UGA_Murdock_StaticTrap::PlacementCancelled);
	WaitPlaceTrap->ReadyForActivation();

	AGameplayAbilityTargetActor* TargetActor;
	WaitPlaceTrap->BeginSpawningActor(this, TargetActorClass, TargetActor);
	ATargetActor_GroundPick* GroundPickActor = Cast<ATargetActor_GroundPick>(TargetActor);
	if ( GroundPickActor )
	{
		GroundPickActor->SetShouldDrawDebug(ShouldDrawDebug());
		GroundPickActor->SetTargetAreaRadius(TrapAreaRadius);
		GroundPickActor->SetTargetTraceRange(TrapThowRange);
	}
	WaitPlaceTrap->FinishSpawningActor(this, TargetActor);

	AddSkillAimEffect();
}

void UGA_Murdock_StaticTrap::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	RemoveSkillAimEffect();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

FGameplayTag UGA_Murdock_StaticTrap::GetThrowTag()
{
	return FGameplayTag::RequestGameplayTag("Ability.Murdock.Trap.Throw");
}

void UGA_Murdock_StaticTrap::PlaceTrap(const FGameplayAbilityTargetDataHandle& Data)
{
	if ( !K2_CommitAbility() )
	{
		K2_EndAbility();
		return;
	}

	RemoveSkillAimEffect();

	if ( PlayTargetingMontage )
	{
		PlayTargetingMontage->OnCancelled.RemoveAll(this);
		PlayTargetingMontage->OnInterrupted.RemoveAll(this);
	}

	int32 LastIndex = Data.Data.Num() - 1;
	PendingTrapSpawnLocation = UAbilitySystemBlueprintLibrary::GetHitResultFromTargetData(Data, LastIndex).ImpactPoint;

	if ( K2_HasAuthority() || IsLocallyControlled() )
	{
		UAbilityTask_PlayMontageAndWait* PlayThrowMontage = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, ThrowingMontage);
		PlayThrowMontage->OnCancelled.AddDynamic(this, &UGA_Murdock_StaticTrap::K2_EndAbility);
		PlayThrowMontage->ReadyForActivation();
	}

	if ( K2_HasAuthority() )
	{
		UAbilityTask_WaitGameplayEvent* WaitThrowEvent = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, GetThrowTag());
		WaitThrowEvent->EventReceived.AddDynamic(this, &UGA_Murdock_StaticTrap::ThrowTagReceived);
		WaitThrowEvent->ReadyForActivation();
	}
}

void UGA_Murdock_StaticTrap::PlacementCancelled(const FGameplayAbilityTargetDataHandle& Data)
{
	K2_EndAbility();
}

void UGA_Murdock_StaticTrap::AddSkillAimEffect()
{
	SkillAimEffectHandle = BP_ApplyGameplayEffectToOwner(SkillAimEffect);
}

void UGA_Murdock_StaticTrap::RemoveSkillAimEffect()
{
	if(SkillAimEffectHandle.IsValid() )
	{
		BP_RemoveGameplayEffectFromOwnerWithHandle(SkillAimEffectHandle);
	}
}

void UGA_Murdock_StaticTrap::ThrowTagReceived(FGameplayEventData Payload)
{
	if ( K2_HasAuthority() && TrapActorClass )
	{
		AActor* OwnerAvatarActor = GetAvatarActorFromActorInfo();
		UAbilitySystemComponent* OwnerASC = GetAbilitySystemComponentFromActorInfo();

		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = OwnerAvatarActor;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		ATA_StaticTrap* NewTrap = GetWorld()->SpawnActorDeferred<ATA_StaticTrap>(TrapActorClass, FTransform(PendingTrapSpawnLocation), OwnerAvatarActor);
		if ( NewTrap )
		{
			int32 AbilityLevel = GetAbilityLevel(CurrentSpecHandle, CurrentActorInfo);
			float TrapDamage = GetLevelIndexedDamage(TrapBaseDamagePerLevel, AbilityLevel);

			NewTrap->ConfigureTrap(
				TrapAreaRadius,
				TrapLifetime,
				GetOwnerTeamId(),
				OwnerASC,
				TrapDamageEffect,
				TrapDamage,
				TrapDamageScalingRatio,
				AbilityLevel,
				BindEffect
			);

			UGameplayStatics::FinishSpawningActor(NewTrap, FTransform(PendingTrapSpawnLocation));

			SpawnedTrapActor = NewTrap;
		}
	}

	K2_EndAbility();
}