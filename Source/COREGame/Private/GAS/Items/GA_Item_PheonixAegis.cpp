// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Items/GA_Item_PheonixAegis.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GAS/CAbilitySystemStatics.h"
#include "GAS/Items/CItemStatics.h"
#include "GAS/CAttributeSet.h"
#include "Engine/OverlapResult.h"

UGA_Item_PheonixAegis::UGA_Item_PheonixAegis()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

	ActivationBlockedTags.RemoveTag(UCAbilitySystemStatics::GetStunStatusTag());

	ActivationRequiredTags.AddTag(TAG_Status_Health_Below40);

	FAbilityTriggerData TriggerData;
	TriggerData.TriggerSource = EGameplayAbilityTriggerSource::OwnedTagAdded;
	TriggerData.TriggerTag = TAG_Status_Health_Below40;
	AbilityTriggers.Add(TriggerData);
}

void UGA_Item_PheonixAegis::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	UAbilitySystemComponent* OwnerASC = GetAbilitySystemComponentFromActorInfo();

	if ( !HasAuthority(&ActivationInfo) || !OwnerASC || !K2_CommitAbility() )
	{
		K2_EndAbility();
		return;
	}

	if ( RegenBuffEffect )
	{
		BP_ApplyGameplayEffectToOwner(RegenBuffEffect, GetAbilityLevel());
	}

	if ( AuraCueTag.IsValid() )
	{
		OwnerASC->AddGameplayCue(AuraCueTag);
	}

	RemainingTicks = TickCount;
	GetWorld()->GetTimerManager().SetTimer(AuraTickTimerHandle, this, &UGA_Item_PheonixAegis::TickAura, TickInterval, true);
}

void UGA_Item_PheonixAegis::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	if ( UWorld* World = GetWorld() )
	{
		World->GetTimerManager().ClearTimer(AuraTickTimerHandle);
	}

	if ( AuraCueTag.IsValid() )
	{
		if ( UAbilitySystemComponent* OwnerASC = GetAbilitySystemComponentFromActorInfo() )
		{
			OwnerASC->RemoveGameplayCue(AuraCueTag);
		}
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_Item_PheonixAegis::TickAura()
{
	AActor* OwnerActor = GetOwningActorFromActorInfo();
	UAbilitySystemComponent* OwnerASC = GetAbilitySystemComponentFromActorInfo();
	if ( !OwnerActor || !OwnerASC || UCAbilitySystemStatics::IsActorDead(OwnerActor))
	{
		K2_EndAbility();
		return;
	}

	bool bFound = false;
	const float MaxHealth = OwnerASC->GetGameplayAttributeValue(UCAttributeSet::GetMaxHealthAttribute(), bFound);
	const float Damage = BaseDamagePerTick + MaxHealth * MaxHealthRatioPerTick;

	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(PheonixAegis), false, OwnerActor);
	GetWorld()->OverlapMultiByObjectType(Overlaps, OwnerActor->GetActorLocation(), FQuat::Identity, FCollisionObjectQueryParams(ECC_Pawn), FCollisionShape::MakeSphere(AuraRadius), Params);

	TSet<AActor*> Processed;
	for ( const FOverlapResult& Overlap : Overlaps )
	{
		AActor* Other = Overlap.GetActor();
		if ( !Other || Processed.Contains(Other) )
			continue;

		Processed.Add(Other);

		if ( !IsActorTeamAttitude(Other, ETeamAttitude::Hostile) || UCAbilitySystemStatics::IsActorDead(Other) )
			continue;
		
		if ( UAbilitySystemComponent* OtherASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Other) )
		{
			UCItemStatics::ApplyItemMagicDamage(OwnerASC, OtherASC, Damage);
		}
	}

	if ( --RemainingTicks <= 0 )
	{
		K2_EndAbility();
	}
}
