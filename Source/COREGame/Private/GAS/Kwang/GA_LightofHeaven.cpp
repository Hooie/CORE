// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Kwang/GA_LightofHeaven.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "GenericTeamAgentInterface.h"
#include "Kismet/KismetSystemLibrary.h"

UGA_LightofHeaven::UGA_LightofHeaven()
{
	BlockAbilitiesWithTag.AddTag(FGameplayTag::RequestGameplayTag("Ability.BasicAttack"));
}

void UGA_LightofHeaven::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!HasAuthorityOrPredictionKey(ActorInfo, &ActivationInfo))
	{
		K2_EndAbility();
		return;
	}

	if (!K2_CommitAbility())
	{
		K2_EndAbility();
		return;
	}

	UAnimMontage* Montage = ActivateMontage;
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (ASC && ASC->HasMatchingGameplayTag(GetNoWeaponTag()))
	{
		Montage = UnarmedActivateMontage;
	}

	UAbilityTask_PlayMontageAndWait* PlayActivateMontage = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(this, NAME_None, Montage);
	PlayActivateMontage->OnBlendOut.AddDynamic(this, &UGA_LightofHeaven::K2_EndAbility);
	PlayActivateMontage->OnCancelled.AddDynamic(this, &UGA_LightofHeaven::K2_EndAbility);
	PlayActivateMontage->OnInterrupted.AddDynamic(this, &UGA_LightofHeaven::K2_EndAbility);
	PlayActivateMontage->OnCompleted.AddDynamic(this, &UGA_LightofHeaven::K2_EndAbility);
	PlayActivateMontage->ReadyForActivation();

	UAbilityTask_WaitGameplayEvent* WaitActivateEvent = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, GetActivateEventTag(), nullptr, false, false);
	WaitActivateEvent->EventReceived.AddDynamic(this, &UGA_LightofHeaven::GrantBuffAndDealDamage);
	WaitActivateEvent->ReadyForActivation();
}

FGameplayTag UGA_LightofHeaven::GetActivateEventTag()
{
	return FGameplayTag::RequestGameplayTag("Event.Activate");
}

FGameplayTag UGA_LightofHeaven::GetNoWeaponTag()
{
	return FGameplayTag::RequestGameplayTag("Status.NoWeapon");
}

void UGA_LightofHeaven::GrantBuffAndDealDamage(FGameplayEventData Data)
{
	if (!K2_HasAuthority())
	{
		return;
	}

	BP_ApplyGameplayEffectToOwner(BuffEffect, GetAbilityLevel(CurrentSpecHandle, CurrentActorInfo));

	TArray<FHitResult> HitResults;

	TArray<TEnumAsByte<EObjectTypeQuery>> types;
	types.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));

	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(GetOwningActorFromActorInfo());

	EDrawDebugTrace::Type DrawDebug = ShouldDrawDebug() ? EDrawDebugTrace::ForDuration : EDrawDebugTrace::None;

	UKismetSystemLibrary::SphereTraceMultiForObjects(
		GetWorld(),
		GetOwningActorFromActorInfo()->GetActorLocation(),
		GetOwningActorFromActorInfo()->GetActorLocation(),
		DamageRange,
		types,
		false,
		ActorsToIgnore,
		DrawDebug,
		HitResults,
		true,
		FLinearColor::Red, FLinearColor::Green,
		4.f
	);

	IGenericTeamAgentInterface* OwnerTeamInterface = Cast<IGenericTeamAgentInterface>(GetOwningActorFromActorInfo());
	if (OwnerTeamInterface)
	{
		int32 AbilityLevel = GetAbilityLevel(CurrentSpecHandle, CurrentActorInfo);
		float BaseDamage = GetLevelIndexedDamage(BaseDamagePerLevel, AbilityLevel);

		FGameplayEffectSpecHandle DamageSpec = MakeScaledDamageSpec(DamageEffect, BaseDamage, ScalingRatio, AbilityLevel);

		for (const FHitResult& HitResult : HitResults)
		{
			if (OwnerTeamInterface->GetTeamAttitudeTowards(*HitResult.GetActor()) == ETeamAttitude::Hostile)
			{
				ApplyGameplayEffectToHitResultActor(HitResult, DamageSpec);
			}
		}
	}

	K2_EndAbility();
}