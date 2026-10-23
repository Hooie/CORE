// Fill out your copyright notice in the Description page of Project Settings.


#include "GAS/Items/GA_Item_AreaEffect.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GAS/CAbilitySystemStatics.h"
#include "Engine/OverlapResult.h"

UGA_Item_AreaEffect::UGA_Item_AreaEffect()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
}

void UGA_Item_AreaEffect::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	UAbilitySystemComponent* OwnerASC = GetAbilitySystemComponentFromActorInfo();
	AActor* OwnerActor = GetOwningActorFromActorInfo();

	if ( !K2_HasAuthority() || !OwnerASC || !OwnerActor || !K2_CommitAbility() )
	{
		K2_EndAbility();
		return;
	}

	if ( CasterCueTag.IsValid() )
	{
		FGameplayCueParameters Params;
		Params.Location = OwnerActor->GetActorLocation();
		Params.RawMagnitude = Radius;

		OwnerASC->ExecuteGameplayCue(CasterCueTag, Params);
	}

	if ( bIncludeSelf && TargetAttitude == ETeamAttitude::Friendly )
	{
		ApplyEffectsTo(OwnerASC, OwnerASC, OwnerActor);
	}

	TArray<FOverlapResult> Overlaps;
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(ItemAreaEffect), false, OwnerActor);
	GetWorld()->OverlapMultiByObjectType(Overlaps, OwnerActor->GetActorLocation(), FQuat::Identity, FCollisionObjectQueryParams(ECC_Pawn), FCollisionShape::MakeSphere(Radius), QueryParams);

	TSet<AActor*> Processed;
	for ( const FOverlapResult& Overlap : Overlaps )
	{
		AActor* Other = Overlap.GetActor();

		if ( !Other || Processed.Contains(Other) )
			continue;

		Processed.Add(Other);

		if ( UCAbilitySystemStatics::IsActorDead(Other) || !IsActorTeamAttitude(Other, TargetAttitude) )
			continue;

		if ( bHeroesOnly && !UCAbilitySystemStatics::IsHero(Other) )
			continue;

		if ( UAbilitySystemComponent* OtherASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Other) )
		{
			ApplyEffectsTo(OwnerASC, OtherASC, Other);
		}
	}

	K2_EndAbility();
}

void UGA_Item_AreaEffect::ApplyEffectsTo(UAbilitySystemComponent* OwnerASC, UAbilitySystemComponent* TargetASC, AActor* TargetActor)
{
	for ( const TSubclassOf<UGameplayEffect>& Effect : EffectsToApply )
	{
		if ( !Effect )
			continue;

		FGameplayEffectSpecHandle Spec = OwnerASC->MakeOutgoingSpec(Effect, GetAbilityLevel(), OwnerASC->MakeEffectContext());
		if ( Spec.IsValid() )
		{
			OwnerASC->ApplyGameplayEffectSpecToTarget(*Spec.Data.Get(), TargetASC);
		}
	}

	if ( TargetCueTag.IsValid() && TargetActor )
	{
		FGameplayCueParameters Params;
		Params.Location = TargetActor->GetActorLocation();
		TargetASC->ExecuteGameplayCue(TargetCueTag, Params);
	}
}
