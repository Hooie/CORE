// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Items/GA_Item_AllyTargetBase.h"
#include "Abilities/Tasks/AbilityTask_WaitTargetData.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GAS/CAbilitySystemStatics.h"
#include "GAS/Muriel/TA_AllySelector.h"

UGA_Item_AllyTargetBase::UGA_Item_AllyTargetBase()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UGA_Item_AllyTargetBase::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if ( !HasAuthorityOrPredictionKey(ActorInfo, &ActivationInfo) || !SelectorClass )
	{
		K2_EndAbility();
		return;
	}

	// 쿨다운/비용은 확인만, 실제 소모는 대상 확정 시
	if ( !K2_CheckAbilityCost() || !K2_CheckAbilityCooldown() )
	{
		K2_EndAbility();
		return;
	}

	UAbilityTask_WaitTargetData* WaitTargetData = UAbilityTask_WaitTargetData::WaitTargetData(this, NAME_None, EGameplayTargetingConfirmation::UserConfirmed, SelectorClass);
	WaitTargetData->ValidData.AddDynamic(this, &UGA_Item_AllyTargetBase::OnTargetConfirmed);
	WaitTargetData->Cancelled.AddDynamic(this, &UGA_Item_AllyTargetBase::OnTargetCancelled);
	WaitTargetData->ReadyForActivation();

	AGameplayAbilityTargetActor* SpawnedTargetActor = nullptr;
	if ( WaitTargetData->BeginSpawningActor(this, SelectorClass, SpawnedTargetActor) )
	{
		if ( ATA_AllySelector* AllySelector = Cast<ATA_AllySelector>(SpawnedTargetActor) )
		{
			AllySelector->SetSelectionRules(TargetRange, bAllowSelfTarget);
		}

		WaitTargetData->FinishSpawningActor(this, SpawnedTargetActor);
	}
}

void UGA_Item_AllyTargetBase::OnTargetConfirmed(const FGameplayAbilityTargetDataHandle& TargetDataHandle)
{
	AActor* Target = UAbilitySystemBlueprintLibrary::GetHitResultFromTargetData(TargetDataHandle, 0).GetActor();

	if ( !IsValidTarget(Target) || !K2_CommitAbility() )
	{
		K2_EndAbility();
		return;
	}

	if ( K2_HasAuthority() )
	{
		UAbilitySystemComponent* OwnerASC = GetAbilitySystemComponentFromActorInfo();
		UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);

		if ( OwnerASC && TargetASC )
		{
			ApplyToTarget(OwnerASC, TargetASC, Target);

			if ( CasterCueTag.IsValid() )
			{
				FGameplayCueParameters CasterCueParams;
				CasterCueParams.Location = GetAvatarActorFromActorInfo()->GetActorLocation();
				OwnerASC->ExecuteGameplayCue(CasterCueTag, CasterCueParams);
			}

			if ( TargetCueTag.IsValid() )
			{
				FGameplayCueParameters TargetCueParams;
				TargetCueParams.Location = Target->GetActorLocation();
				TargetASC->ExecuteGameplayCue(TargetCueTag, TargetCueParams);
			}
		}
	}

	K2_EndAbility();
}

void UGA_Item_AllyTargetBase::OnTargetCancelled(const FGameplayAbilityTargetDataHandle& TargetDataHandle)
{
	K2_EndAbility();
}

bool UGA_Item_AllyTargetBase::IsValidTarget(const AActor* Target) const
{
	const AActor* OwnerActor = GetAvatarActorFromActorInfo();

	if ( !Target || !OwnerActor || UCAbilitySystemStatics::IsActorDead(Target) || !UCAbilitySystemStatics::IsHero(Target) )
		return false;

	if ( Target == OwnerActor )
		return bAllowSelfTarget;

	if ( !IsActorTeamAttitude(Target, ETeamAttitude::Friendly) )
		return false;

	return FVector::DistSquared(OwnerActor->GetActorLocation(), Target->GetActorLocation()) <= FMath::Square(TargetRange + ServerRangeTolerance);
}